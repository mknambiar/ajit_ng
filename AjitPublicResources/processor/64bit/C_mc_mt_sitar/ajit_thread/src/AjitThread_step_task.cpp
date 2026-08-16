#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <mutex>
#include <string>
#include "../include/AjitThread.h"
#include "../include/CoroutineIcacheMmu.h"
#include "../include/ExecuteStepTask.h"
#include "../../shim/deep_fetch_step.h"
extern "C" {
#include "AjitThreadUtils.h"
#include "Core.h"
#include "ThreadInterface.h"
#include "Ancillary.h"
#include "monitorLogger.h"
#include "RequestTypeValues.h"
#include "Decode.h"
#include "Execute.h"
#include "Flags.h"
#include "Opcodes.h"
#include "Traps.h"
#include "Ajit_Hardware_Configuration.h"

// Implemented in AjitThread.c (not declared in headers).
void writeBackResult(RegisterFile* rf, uint8_t dest_reg, uint32_t result_h, uint32_t result_l,
                     uint8_t flags, uint8_t cwp, uint32_t pc, StateUpdateFlags* reg_update_flags);
uint8_t isBranchInstruction(Opcode op, InstructionType type);
void increment_instruction_count(ThreadState* s);
void clearStateUpdateFlags(ThreadState* s);
uint32_t completeFPExecution(ThreadState* s,
                             Opcode opcode,
                             uint32_t operand2_3, uint32_t operand2_2,
                             uint32_t operand2_1, uint32_t operand2_0,
                             uint32_t operand1_3, uint32_t operand1_2,
                             uint32_t operand1_1, uint32_t operand1_0,
                             uint8_t dest_reg, StatusRegisters *ptr, uint32_t trap_vector);

extern int global_enable_statistic_collection;
extern int global_stat_collection_trigger_pc;
}

namespace {

bool shouldTracePcWindow(uint32_t pc)
{
  static int inited = 0;
  static uint32_t start0 = 0, end0 = 0, start1 = 0, end1 = 0;

  if (!inited) {
    auto parse_hex_env = [](const char* name, uint32_t* out) {
      const char* env = std::getenv(name);
      if ((env == nullptr) || (env[0] == '\0')) {
        return false;
      }
      char* endp = nullptr;
      unsigned long v = std::strtoul(env, &endp, 0);
      if (endp == env) {
        return false;
      }
      *out = (uint32_t) v;
      return true;
    };

    (void) parse_hex_env("AJIT_STEP_PC_WINDOW0_START", &start0);
    (void) parse_hex_env("AJIT_STEP_PC_WINDOW0_END", &end0);
    (void) parse_hex_env("AJIT_STEP_PC_WINDOW1_START", &start1);
    (void) parse_hex_env("AJIT_STEP_PC_WINDOW1_END", &end1);
    inited = 1;
  }

  const bool in0 = ((start0 != 0u) && (end0 != 0u) && (pc >= start0) && (pc < end0));
  const bool in1 = ((start1 != 0u) && (end1 != 0u) && (pc >= start1) && (pc < end1));
  return (in0 || in1);
}

void emitWriteTraceIfEnabled(const ThreadState* s)
{
  static std::mutex wtrace_mu;
  static bool wtrace_inited = false;
  static bool wtrace_enabled = false;
  static std::string wtrace_base;
  static std::map<uint64_t, FILE*> wtrace_files;
  static std::map<uint64_t, uint64_t> wtrace_count;

  if (!wtrace_inited) {
    const char* env = std::getenv("AJIT_TRACE_W");
    if (env && env[0]) {
      wtrace_enabled = true;
      wtrace_base = env;
    }
    wtrace_inited = true;
  }
  if (!wtrace_enabled || s == nullptr) {
    return;
  }

  std::lock_guard<std::mutex> guard(wtrace_mu);
  const uint64_t key = (((uint64_t) s->core_id) << 32) | (uint64_t) s->thread_id;

  FILE* fp = nullptr;
  auto fit = wtrace_files.find(key);
  if (fit == wtrace_files.end()) {
    if (wtrace_base == "stdout") {
      fp = stdout;
    } else {
      char path[4096];
      std::snprintf(path, sizeof(path), "%s.%u_%u",
                    wtrace_base.c_str(), s->core_id, s->thread_id);
      fp = std::fopen(path, "w");
    }

    if (fp == nullptr) {
      std::fprintf(stderr,
                   "WARN: could not open AJIT_TRACE_W target for c%u t%u\n",
                   s->core_id, s->thread_id);
      return;
    }

    std::fprintf(fp,
                 "//============== C model register-write trace (core,thread=%u,%u) ============\n",
                 s->core_id, s->thread_id);
    std::fprintf(fp,
                 "//=========================================================================\n");
    std::fflush(fp);
    wtrace_files[key] = fp;
    wtrace_count[key] = 0;
  } else {
    fp = fit->second;
  }

  const StateUpdateFlags* f = &(s->reg_update_flags);
  const uint32_t reg_write_log = assembleRegisterWriteSignature(
      f->psr_updated, s->status_reg.psr,
      f->wim_updated, s->status_reg.wim,
      f->tbr_updated, s->status_reg.tbr,
      f->y_updated, s->status_reg.y,
      f->gpr_updated, f->double_word_write,
      f->reg_id, f->reg_val_high, f->reg_val_low);

  const uint32_t fp_reg_write_log = assembleFpRegisterWriteSignature(
      f->fpreg_updated, 0,
      (f->fsr_updated | f->ftt_updated | f->fcc_updated), 0,
      f->fsr_updated, f->ftt_updated, f->fcc_updated,
      f->fpreg_double_word_write, f->fpreg_id,
      f->fsr_val, f->fpreg_val_high, f->fpreg_val_low);

  const uint32_t store_log = assembleStoreSignature(
      f->store_active, f->store_asi, f->store_byte_mask,
      f->store_double_word, f->store_addr,
      f->store_word_high, f->store_word_low);

  const uint32_t pc_log = f->pc;
  const uint64_t count = wtrace_count[key];
  std::fprintf(fp,
               "%llu. PC=%08x, Reg-Log=%08x Fp-Reg-log=%08x  Store-log= %08x\n",
               (unsigned long long) count,
               pc_log, reg_write_log, fp_reg_write_log, store_log);
  std::fflush(fp);
  wtrace_count[key] = count + 1;
}

StepTaskT<uint8_t> fetchInstruction(CoroutineOwner* owner,
                                    ThreadState* s,
                                    uint8_t addr_space,
                                    uint32_t addr,
                                    uint32_t* inst,
                                    uint32_t* mmu_fsr)
{
  uint8_t mae_value = 0;
  uint64_t ipair = 0;
  uint32_t local_mmu_fsr = 0;
  uint8_t acc = 0;

  int is_buffer_hit = 0;
  if (s->i_buffer != nullptr) {
    is_buffer_hit = lookupInstructionDataBuffer(s->i_buffer,
                                                (addr & 0xfffffff8u),
                                                &acc,
                                                &ipair);
    is_buffer_hit = is_buffer_hit && privilegesOk((uint8_t) (addr_space & 0x7f), 1, 1, acc);
  }

  if (!is_buffer_hit) {
    StepTaskT<void> ifetch_task = cpuIcacheAccess(owner,
                                                  (int) s->core_id,
                                                  (int) s->thread_id,
                                                  0,
                                                  s->mmu_state,
                                                  s->icache,
                                                  (uint8_t) (addr_space | 0x80),
                                                  (addr & ~0x7u),
                                                  REQUEST_TYPE_IFETCH,
                                                  0xff,
                                                  &mae_value,
                                                  &ipair,
                                                  &local_mmu_fsr);
    CO_AWAIT_OWNED(owner, ifetch_task);

    if (s->i_buffer != nullptr) {
      if ((mae_value & 0x3u) == 0) {
        uint8_t cacheable = (uint8_t) ((mae_value >> 7) & 0x1u);
        if (cacheable) {
          uint8_t local_acc = (uint8_t) ((mae_value >> 4) & 0x7u);
          insertIntoInstructionDataBuffer(s->i_buffer,
                                          (addr & 0xfffffff8u),
                                          local_acc,
                                          ipair);
        }
      }
    }
  }

  setPageBit((CoreState*) s->parent_core_state, addr);
  if (getBit32(addr, 2)) {
    *inst = (uint32_t) (ipair & 0xffffffffu);
  } else {
    *inst = (uint32_t) (ipair >> 32);
  }

  if (mmu_fsr) {
    *mmu_fsr = local_mmu_fsr;
  }

  co_return (uint8_t) (mae_value & 0x1);
}

StepTaskT<void> updateMmuFsrFarStep(CoroutineOwner* owner,
                                    ThreadState* s,
                                    uint32_t mmu_fsr,
                                    uint32_t mmu_far)
{
  uint64_t data64 = 0;
  data64 = setSlice64(data64, 63, 32, mmu_fsr);
  data64 = setSlice64(data64, 31, 0, mmu_far);

  uint8_t mae = 0;
  uint64_t rd = 0;
  StepTaskT<void> wtask = cpuDcacheAccess(owner,
                                          (int) s->core_id,
                                          (int) s->thread_id,
                                          s->mmu_state->MmuContextRegister[s->thread_id],
                                          s->mmu_state,
                                          s->dcache,
                                          0x0,
                                          0x0,
                                          REQUEST_TYPE_WRFSRFAR,
                                          0xff,
                                          data64,
                                          &mae,
                                          &rd);
  CO_AWAIT_OWNED(owner, wtask);
}

} // namespace

extern "C" uint64_t ajit_step_mem_class_total = 0;
extern "C" uint64_t ajit_step_op3_mem_counts[64] = {0};
extern "C" uint64_t ajit_step_total_decoded = 0;
extern "C" uint64_t ajit_step_format_counts[4] = {0, 0, 0, 0};
extern "C" uint64_t ajit_step_exec_failures = 0;
extern "C" uint64_t ajit_step_fetch_ok = 0;
extern "C" uint64_t ajit_step_fetch_trap = 0;
extern "C" uint64_t ajit_step_pc_trace_printed = 0;

extern "C" void ajit_step_dump_opcode_summary(void)
{
  std::fprintf(stderr,
               "BRIDGE-SUMMARY decoded-total=%llu fmt0=%llu fmt1=%llu fmt2=%llu fmt3=%llu exec-fail=%llu fetch-ok=%llu fetch-trap=%llu\n",
               (unsigned long long) ajit_step_total_decoded,
               (unsigned long long) ajit_step_format_counts[0],
               (unsigned long long) ajit_step_format_counts[1],
               (unsigned long long) ajit_step_format_counts[2],
               (unsigned long long) ajit_step_format_counts[3],
               (unsigned long long) ajit_step_exec_failures,
               (unsigned long long) ajit_step_fetch_ok,
               (unsigned long long) ajit_step_fetch_trap);
  std::fprintf(stderr,
               "BRIDGE-SUMMARY mem-class-total=%llu\n",
               (unsigned long long) ajit_step_mem_class_total);
  for (int op3 = 0; op3 < 64; ++op3) {
    if (ajit_step_op3_mem_counts[op3] != 0) {
      std::fprintf(stderr,
                   "BRIDGE-SUMMARY mem-op3[0x%02x]=%llu\n",
                   op3,
                   (unsigned long long) ajit_step_op3_mem_counts[op3]);
    }
  }
}

StepTaskT<int> ajit_thread(CoroutineOwner* owner, ThreadState* state_ptr)
{
  if (state_ptr == nullptr) {
    co_return 0;
  }

  const int trace = (std::getenv("AJIT_STEP_TRACE") != nullptr);
  const char* pc_trace_env = std::getenv("AJIT_STEP_PC_TRACE_N");
  const uint64_t pc_trace_limit =
      (pc_trace_env && pc_trace_env[0]) ? (uint64_t) std::strtoull(pc_trace_env, nullptr, 0) : 0ull;
  if (trace) {
    std::fprintf(stderr,
                 "STEP-TASK-BEG c%u t%u sim=%llu phase=%llu pc=0x%08x npc=0x%08x\n",
                 state_ptr->core_id, state_ptr->thread_id,
                 (unsigned long long) state_ptr->sitar_sim_time,
                 (unsigned long long) (state_ptr->sitar_sim_time & 0x1ull),
                 state_ptr->status_reg.pc, state_ptr->status_reg.npc);
  }

  // Mirror legacy per-step control flow before fetch/decode.
  const uint8_t reset_mode = (state_ptr->mode == _RESET_MODE_);
  const uint8_t error_mode = (state_ptr->mode == _ERROR_MODE_);
  const uint8_t execute_mode = (state_ptr->mode == _EXECUTE_MODE_);

  const uint8_t reset_in1 = (getBpReset(state_ptr) == 0);
  const uint8_t reset_in_reset = (reset_mode && reset_in1);
  if (reset_in_reset) {
    state_ptr->mode = _EXECUTE_MODE_;
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _RESET_TRAP_, 1);
  }

  if (error_mode) {
    setPbError(state_ptr, 1);
    co_return 1;
  }

  const uint8_t reset_in2 = (getBpReset(state_ptr) == 1);
  const uint8_t reset_in_error = (error_mode && reset_in2);
  if (reset_in_error) {
    state_ptr->mode = _RESET_MODE_;
    setPbError(state_ptr, 0);
  }

  const uint8_t reset_in3 = (getBpReset(state_ptr) == 1);
  const uint8_t reset_in_execute = (execute_mode && reset_in3);
  if (reset_in_execute) {
    state_ptr->mode = _RESET_MODE_;
  }
  if (reset_in_execute || reset_in_error) {
    resetThreadState(state_ptr);
  }

  const uint8_t enable_trap =
      (!reset_in_execute && (getBit32(state_ptr->status_reg.psr, 5) == 1));
  uint8_t interrupt_level = 0;
  if (state_ptr->mode == _EXECUTE_MODE_) {
    interrupt_level = getBpIRL(state_ptr);
  }
  const uint8_t is_interrupt =
      (enable_trap && ((interrupt_level == 15) ||
       (interrupt_level > getSlice32(state_ptr->status_reg.psr, 11, 8))));
  if (is_interrupt) {
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
    state_ptr->interrupt_level = interrupt_level;
  }

  const uint8_t skip_trap_execution =
      !((state_ptr->mode == _EXECUTE_MODE_) && (getBit32(state_ptr->trap_vector, _TRAP_) == 1));
  if (!skip_trap_execution) {
    if (global_enable_statistic_collection) {
      state_ptr->num_traps++;
    }
    executeTrap(state_ptr,
                &(state_ptr->trap_vector), &(state_ptr->status_reg.psr),
                &(state_ptr->status_reg.tbr), &(state_ptr->interrupt_level),
                &(state_ptr->mode), state_ptr->ticc_trap_type,
                &(state_ptr->status_reg.pc), &(state_ptr->status_reg.npc));
    if (global_enable_statistic_collection) {
      StepTask penalty = wait_penalty_cycles(owner, TRAP_PENALTY);
      CO_AWAIT_OWNED(owner, penalty);
    }
  }

  // Match legacy ajit_thread_step: derive instruction address-space from PSR.S.
  // PSR bit-7 == 0 -> supervisor instruction ASI (0x8), else user instruction ASI (0x9).
  state_ptr->addr_space = (getBit32(state_ptr->status_reg.psr, 7) == 0) ? 8 : 9;
  uint8_t skip_fetch = !(state_ptr->mode == _EXECUTE_MODE_);
  if (skip_fetch) {
    co_return 1;
  }

  clearStateUpdateFlags(state_ptr);
  state_ptr->reg_update_flags.pc = state_ptr->status_reg.pc;
  if (!global_enable_statistic_collection &&
      (state_ptr->status_reg.pc == (uint32_t) global_stat_collection_trigger_pc)) {
    global_enable_statistic_collection = 1;
  }

  uint8_t fetch_trap = 0;
  CO_AWAIT_OWNED_TO(fetch_trap, owner,
                    fetchInstruction(owner,
                                     state_ptr,
                                     state_ptr->addr_space,
                                     state_ptr->status_reg.pc,
                                     &state_ptr->instruction,
                                     &state_ptr->mmu_fsr));
  uint8_t skip_decode = fetch_trap;
  if (fetch_trap != 0) {
    // Legacy-style post-fetch handling.
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _MAE_, fetch_trap);
    if (getBit32(state_ptr->trap_vector, _ANNUL_)) {
      state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _ANNUL_, 0);
      state_ptr->status_reg.pc = state_ptr->status_reg.npc;
      state_ptr->status_reg.npc = state_ptr->status_reg.npc + 4;
    } else {
      state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
      state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _INSTRUCTION_ACCESS_EXCEPTION_, 1);
    }
    ajit_step_fetch_trap++;
    if (trace) {
      std::fprintf(stderr, "STEP-TASK-NOK c%u t%u sim=%llu trap=0x%x\n",
                   state_ptr->core_id, state_ptr->thread_id,
                   (unsigned long long) state_ptr->sitar_sim_time,
                   fetch_trap);
    }
  }
  if (!skip_decode) {
    ajit_step_fetch_ok++;
    state_ptr->num_ifetches++;
  } else {
    co_return 1;
  }

  // Legacy behavior: reflect IFETCH MMU FSR/FAR through DCACHE path.
  if (state_ptr->mmu_fsr != 0) {
    StepTaskT<void> mmu_fsr_far_task = updateMmuFsrFarStep(owner,
                                                           state_ptr,
                                                           state_ptr->mmu_fsr,
                                                           state_ptr->status_reg.pc);
    CO_AWAIT_OWNED(owner, mmu_fsr_far_task);
  }

  // Legacy-like flow: decode -> readOperands -> execute -> writeback.
  Opcode opcode = _NOP_;
  InstructionType inst_type = _UNKNOWN_;
  uint8_t rs1 = 0, rs2 = 0, rd = 0, i = 0, a = 0, asi = 0, software_trap = 0;
  uint8_t shcnt = 0, fp_uimp_inst = 0, vector_data_type = 0, fp_invalid_reg = 0;
  uint16_t simm13 = 0;
  uint32_t disp30 = 0, disp22 = 0;

  uint32_t operand2_3 = 0, operand2_2 = 0, operand2_1 = 0, operand2_0 = 0;
  uint32_t operand1_3 = 0, operand1_2 = 0, operand1_1 = 0, operand1_0 = 0;
  uint32_t data1 = 0, data0 = 0;
  uint32_t result_h = 0, result_l = 0;
  uint8_t flags = 0;
  const uint8_t cwp = (uint8_t) getSlice32(state_ptr->status_reg.psr, 4, 0);

  uint8_t annul_trap = getBit32(state_ptr->trap_vector, _ANNUL_);
  if (annul_trap) {
    // Legacy behavior: annul current instruction and advance PC/NPC.
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _ANNUL_, 0);
    state_ptr->status_reg.pc = state_ptr->status_reg.npc;
    state_ptr->status_reg.npc = state_ptr->status_reg.npc + 4;
  }

  skip_decode = (skip_decode || annul_trap);
  if (!skip_decode) {
    uint32_t trap_vector = state_ptr->trap_vector;
    decodeInstruction(state_ptr->instruction, state_ptr->isa_mode,
                      &opcode, trap_vector, &rs1, &rs2,
                      &simm13, &shcnt, &disp30, &disp22, &software_trap,
                      &rd, &i, &a, &asi, &inst_type,
                      &fp_uimp_inst, &vector_data_type);
    flags = ((a == 1) ? setBit8(flags, _ANNUL_FLAG_, 1) : flags);

    if (fp_uimp_inst) {
      state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
      state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _FP_EXCEPTION_, 1);
      state_ptr->status_reg.fsr = setSlice32(state_ptr->status_reg.fsr, 16, 14, _UNIMPLEMENTED_FPOP_);
    }

    readOperands(state_ptr->register_file,
                 opcode, rs1, rs2, rd, simm13, shcnt, disp30, disp22,
                 software_trap, i,
                 &operand2_3, &operand2_2, &operand2_1, &operand2_0,
                 &operand1_3, &operand1_2, &operand1_1, &operand1_0,
                 inst_type, cwp,
                 &data1, &data0, &fp_invalid_reg);

    if (fp_invalid_reg) {
      state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
      state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _FP_EXCEPTION_, 1);
      state_ptr->status_reg.fsr = setSlice32(state_ptr->status_reg.fsr, 16, 14, _INVALID_FP_REGISTER_);
    }
  }

  if (!skip_decode) {
    const uint32_t format_op = (state_ptr->instruction >> 30) & 0x3u;
    ajit_step_total_decoded++;
    ajit_step_format_counts[format_op]++;
    // Kept only for counters/logging; executeInstruction now decides opcode class internally.
    const int is_memory_format = (format_op == 0x3u);
    const uint32_t op3 = (state_ptr->instruction >> 19) & 0x3fu;
    if ((pc_trace_limit > 0) &&
        (state_ptr->core_id == 0) &&
        (state_ptr->thread_id == 0) &&
        (ajit_step_pc_trace_printed < pc_trace_limit)) {
      std::fprintf(stderr,
                   "STEP-PC c%u t%u sim=%llu pc=0x%08x inst=0x%08x fmt=%u op3=0x%02x\n",
                   state_ptr->core_id, state_ptr->thread_id,
                   (unsigned long long) state_ptr->sitar_sim_time,
                   state_ptr->status_reg.pc, state_ptr->instruction,
                   format_op, op3);
      ajit_step_pc_trace_printed++;
    }
    if ((state_ptr->core_id == 0) &&
        (state_ptr->thread_id == 0) &&
        shouldTracePcWindow(state_ptr->status_reg.pc)) {
      std::fprintf(stderr,
                   "STEP-PC-WINDOW c%u t%u sim=%llu pc=0x%08x inst=0x%08x fmt=%u op3=0x%02x\n",
                   state_ptr->core_id, state_ptr->thread_id,
                   (unsigned long long) state_ptr->sitar_sim_time,
                   state_ptr->status_reg.pc, state_ptr->instruction,
                   format_op, op3);
    }
    if (is_memory_format) {
      ajit_step_mem_class_total++;
      ajit_step_op3_mem_counts[op3]++;
    }
  }
  const uint8_t is_fp_op = ((inst_type == _FPop1_INS_) || (inst_type == _FPop2_INS_));
  uint8_t skip_fp_execute = (skip_decode || !is_fp_op);
  if (!skip_fp_execute) {
    uint64_t fp_penalty = 0;
    if (global_enable_statistic_collection) {
      if (opcode == _FSQRTs_) {
        fp_penalty = FP_SP_SQROOT_PENALTY;
      } else if (opcode == _FSQRTd_) {
        fp_penalty = FP_DP_SQROOT_PENALTY;
      } else if (opcode == _FDIVs_) {
        fp_penalty = FP_SP_DIV_PENALTY;
      } else if (opcode == _FDIVd_) {
        fp_penalty = FP_DP_DIV_PENALTY;
      }
    }
    state_ptr->trap_vector =
        completeFPExecution(state_ptr,
                            opcode,
                            operand2_3, operand2_2, operand2_1, operand2_0,
                            operand1_3, operand1_2, operand1_1, operand1_0,
                            rd, &(state_ptr->status_reg), state_ptr->trap_vector);
    if (fp_penalty != 0) {
      StepTask penalty = wait_penalty_cycles(owner, fp_penalty);
      CO_AWAIT_OWNED(owner, penalty);
    }
  }

  uint8_t skip_execute = (skip_decode || is_fp_op);
  if (!skip_execute) {
    int exec_ok = 0;
    CO_AWAIT_OWNED_TO(exec_ok, owner,
                      executeInstruction(owner,
                                         state_ptr,
                                         trace,
                                         i,
                                         asi,
                                         rd,
                                         rs1,
                                         vector_data_type,
                                         opcode,
                                         operand2_0,
                                         operand2_1,
                                         operand1_0,
                                         operand1_1,
                                         data1,
                                         data0,
                                         &result_h,
                                         &result_l,
                                         &flags));
    if (!exec_ok) {
      ajit_step_exec_failures++;
      co_return 0;
    }
  }
  uint8_t post_execute_trap = getBit32(state_ptr->trap_vector, _TRAP_);
  uint8_t need_write_back = getBit8(flags, _NEED_WRITE_BACK_);
  uint8_t skip_write_back = (skip_execute || post_execute_trap || !need_write_back);
  if (!skip_write_back) {
    const uint8_t wb_cwp = (uint8_t) getSlice32(state_ptr->status_reg.psr, 4, 0);
    writeBackResult(state_ptr->register_file, rd, result_h, result_l,
                    flags, wb_cwp,
                    state_ptr->status_reg.pc,
                    &(state_ptr->reg_update_flags));
  }

  if (state_ptr->status_reg.npc == 0) {
    state_ptr->status_reg.npc = state_ptr->status_reg.pc + 4;
  }
  uint8_t is_branch = isBranchInstruction(opcode, inst_type);
  if (!skip_fetch && (getBit32(state_ptr->trap_vector, _TRAP_) == 0)) {
    increment_instruction_count(state_ptr);
  }
  emitWriteTraceIfEnabled(state_ptr);

  uint8_t update_pc = ((!skip_execute || !skip_fp_execute) && !post_execute_trap && !is_branch);
  if (update_pc) {
    state_ptr->status_reg.pc = state_ptr->status_reg.npc;
    state_ptr->status_reg.npc = state_ptr->status_reg.npc + 4;
  }

  if (trace) {
    std::fprintf(stderr,
                 "STEP-TASK-END c%u t%u sim=%llu inst=0x%08x next_pc=0x%08x trap=0x%08x\n",
                 state_ptr->core_id, state_ptr->thread_id,
                 (unsigned long long) state_ptr->sitar_sim_time,
                 state_ptr->instruction, state_ptr->status_reg.pc,
                 state_ptr->trap_vector);
  }

  co_return 1;
}
