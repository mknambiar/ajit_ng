#include "ExecuteStepTask.h"
#include "CoroutineIcacheMmu.h"
#include "../../shim/modular_access_adapter.h"
#include <cstdio>
#include <cstdlib>

extern "C" {
#include "ASI_values.h"
#include "RequestTypeValues.h"
#include "Execute.h"
#include "ThreadInterface.h"
#include "Flags.h"
#include "Traps.h"
#include "Ancillary.h"
#include "Ajit_Hardware_Configuration.h"

// Declared in Execute.c but not exported via Execute.h.
uint32_t executeTAdd(Opcode op, uint32_t operand1, uint32_t operand2, uint32_t *result,
                     StatusRegisters *status_reg, StateUpdateFlags* reg_update_flags,
                     uint32_t trap_vector, uint8_t *flags);
uint32_t executeTSub(Opcode op, uint32_t operand1, uint32_t operand2, uint32_t *result,
                     StatusRegisters *status_reg, StateUpdateFlags* reg_update_flags,
                     uint32_t trap_vector, uint8_t *flags);
void executeMulStep(uint32_t operand1, uint32_t operand2, uint32_t *result,
                    StatusRegisters *status_reg, StateUpdateFlags* reg_update_flags,
                    uint8_t *flags);
void testAndSetBlockLdstFlags(ThreadState* state, uint8_t byte_flag, uint8_t word_flag);
uint32_t executeCoprocessor(uint32_t trap_vector);

extern int global_enable_statistic_collection;
extern int global_verbose_flag;
}

namespace {

inline void setDataAccessTrap(ThreadState* state_ptr)
{
  if (state_ptr == nullptr) {
    return;
  }
  state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
  state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _DATA_ACCESS_EXCEPTION_, 1);
}

inline uint8_t currentThreadContext(const ThreadState* state_ptr)
{
  if ((state_ptr != nullptr) && (state_ptr->mmu_state != nullptr)) {
    return state_ptr->mmu_state->MmuContextRegister[state_ptr->thread_id];
  }
  return 0;
}

inline uint8_t effectiveMemAddrSpace(const ThreadState* state_ptr, uint32_t op3, uint8_t asi)
{
  // Legacy Execute.c semantics:
  // - non-alternate memory ops use primary spaces: user=10, supervisor=11
  // - alternate memory ops (0x10..0x1f) use ASI[5:0] in supervisor mode
  const uint8_t s = (uint8_t) getBit32(state_ptr->status_reg.psr, 7);
  uint8_t addr_space = (s ? 11u : 10u);
  if ((op3 & 0x30u) == 0x10u) {
    if (s) {
      addr_space = (uint8_t) (asi & 0x3fu);
    }
  }
  return addr_space;
}

inline void recordStoreUpdate(ThreadState* state_ptr,
                              uint8_t addr_space,
                              uint32_t address,
                              bool is_double_word,
                              uint8_t word_byte_mask,
                              uint32_t word_high,
                              uint32_t word_low)
{
  StateUpdateFlags* rf = &(state_ptr->reg_update_flags);
  rf->store_active = 1;
  rf->store_asi = addr_space;
  rf->store_addr = address;
  rf->store_double_word = is_double_word ? 1 : 0;
  rf->store_byte_mask = is_double_word ? 0xffu
                                       : ((address & 0x4u) ? word_byte_mask
                                                           : (uint8_t) (word_byte_mask << 4));
  rf->store_word_low = word_low;
  rf->store_word_high = word_high;
}

StepTaskT<void> dcacheAccessStep(CoroutineOwner* owner,
                                 ThreadState* state_ptr,
                                 int core_id,
                                 int thread_id,
                                 uint8_t thread_context,
                                 MmuState* mmu_state,
                                 WriteThroughAllocateCache* dcache,
                                 uint8_t addr_space,
                                 uint32_t addr,
                                 uint8_t request_type,
                                 uint8_t byte_mask,
                                 uint64_t write_data,
                                 uint8_t* mae,
                                 uint64_t* read_data)
{
  // Keep the last effective data-space ASI for bridge/result checking.
  const uint8_t req = (uint8_t) (request_type & 0x3fu);
  const bool is_data_req = ((req == REQUEST_TYPE_READ) || (req == REQUEST_TYPE_WRITE));
  if ((state_ptr != nullptr) && is_data_req && (addr_space != 0u)) {
    const uint32_t op3 = (state_ptr->instruction >> 19) & 0x3fu;
    const bool is_alternate_mem_op = ((op3 & 0x30u) == 0x10u);
    const bool is_unlock_probe = (addr_space == 0x20u) && (addr == state_ptr->init_pc);

    if ((addr_space == 0x0au) || (addr_space == 0x0bu)) {
      // Normal data-space accesses should not keep a stale alternate ASI.
      state_ptr->last_data_asi_valid = 0u;
    } else if ((addr_space == 0x20u) && is_alternate_mem_op && !is_unlock_probe) {
      // Preserve expected alternate-space ASI visibility for *a paths.
      state_ptr->last_data_asi = 0x20u;
      state_ptr->last_data_asi_valid = 1u;
    }
  }

  // Legacy ThreadInterface path always tags CPU-originated dcache operations
  // with IS_NEW_THREAD. Keep the same behavior in coroutine mode.
  const uint8_t req_type = (uint8_t) (request_type | IS_NEW_THREAD);
  StepTaskT<void> task =
      modular_thread_ports_enabled(core_id, thread_id)
          ? modularCpuDcacheAccess(owner,
                                   core_id,
                                   thread_id,
                                   thread_context,
                                   addr_space,
                                   addr,
                                   req_type,
                                   byte_mask,
                                   write_data,
                                   mae,
                                   read_data)
          : cpuDcacheAccess(owner,
                            core_id,
                            thread_id,
                            thread_context,
                            mmu_state,
                            dcache,
                            addr_space,
                            addr,
                            req_type,
                            byte_mask,
                            write_data,
                            mae,
                            read_data);
  CO_AWAIT_OWNED(owner, task);
}

StepTaskT<int> dcacheReadChecked(CoroutineOwner* owner,
                                 ThreadState* state_ptr,
                                 uint8_t addr_space,
                                 uint8_t request_type,
                                 uint32_t addr,
                                 uint8_t byte_mask,
                                 uint64_t* rd64,
                                 int trace)
{
  const uint32_t op3 = (state_ptr->instruction >> 19) & 0x3fu;
  uint8_t mae_mem = 0;
  StepTaskT<void> rtask = dcacheAccessStep(owner,
                                           state_ptr,
                                           (int) state_ptr->core_id,
                                           (int) state_ptr->thread_id,
                                           currentThreadContext(state_ptr),
                                           state_ptr->mmu_state,
                                           state_ptr->dcache,
                                           addr_space,
                                           addr,
                                           request_type,
                                           byte_mask,
                                           0,
                                           &mae_mem,
                                           rd64);
  CO_AWAIT_OWNED(owner, rtask);
  if (mae_mem) {
    if (trace) {
      std::fprintf(stderr,
                   "STEP-TASK-MEM-RD-FAIL c%u t%u sim=%llu op3=0x%02x\n",
                   state_ptr->core_id, state_ptr->thread_id,
                   (unsigned long long) state_ptr->sitar_sim_time, op3);
    }
    co_return 0;
  }
  co_return 1;
}

StepTaskT<int> dcacheWriteChecked(CoroutineOwner* owner,
                                  ThreadState* state_ptr,
                                  uint8_t addr_space,
                                  uint32_t addr,
                                  uint8_t byte_mask,
                                  uint64_t wr64,
                                  int trace)
{
  const uint32_t op3 = (state_ptr->instruction >> 19) & 0x3fu;
  uint8_t mae_mem = 0;
  uint64_t unused = 0;
  StepTaskT<void> wtask = dcacheAccessStep(owner,
                                           state_ptr,
                                           (int) state_ptr->core_id,
                                           (int) state_ptr->thread_id,
                                           currentThreadContext(state_ptr),
                                           state_ptr->mmu_state,
                                           state_ptr->dcache,
                                           addr_space,
                                           addr,
                                           REQUEST_TYPE_WRITE,
                                           byte_mask,
                                           wr64,
                                           &mae_mem,
                                           &unused);
  CO_AWAIT_OWNED(owner, wtask);
  if (mae_mem) {
    if (trace) {
      std::fprintf(stderr,
                   "STEP-TASK-MEM-WR-FAIL c%u t%u sim=%llu op3=0x%02x\n",
                   state_ptr->core_id, state_ptr->thread_id,
                   (unsigned long long) state_ptr->sitar_sim_time, op3);
    }
    co_return 0;
  }
  co_return 1;
}

StepTaskT<int> dcacheRequestMae(CoroutineOwner* owner,
                                ThreadState* state_ptr,
                                uint8_t addr_space,
                                uint32_t addr,
                                uint8_t request_type,
                                uint8_t byte_mask,
                                uint64_t write_data,
                                uint64_t* read_data)
{
  uint8_t mae_mem = 0;
  uint64_t unused = 0;
  uint64_t* rdp = (read_data != nullptr) ? read_data : &unused;
  StepTaskT<void> task = dcacheAccessStep(owner,
                                          state_ptr,
                                          (int) state_ptr->core_id,
                                          (int) state_ptr->thread_id,
                                          currentThreadContext(state_ptr),
                                          state_ptr->mmu_state,
                                          state_ptr->dcache,
                                          addr_space,
                                          addr,
                                          request_type,
                                          byte_mask,
                                          write_data,
                                          &mae_mem,
                                          rdp);
  CO_AWAIT_OWNED(owner, task);
  co_return (mae_mem ? 0 : 1);
}

StepTaskT<int> executeLdstub(CoroutineOwner* owner,
                             ThreadState* state_ptr,
                             uint8_t asi,
                             uint32_t operand1_0,
                             uint32_t operand2_0,
                             uint32_t* result_l,
                             uint8_t* flags,
                             int trace)
{
  uint32_t eff_addr = (operand1_0 + operand2_0);
  const uint32_t op3 = (state_ptr->instruction >> 19) & 0x3fu;
  const uint8_t s = (uint8_t) getBit32(state_ptr->status_reg.psr, 7);
  if ((op3 == 0x1du) && !s) {
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _PRIVILEGED_INSTRUCTION_, 1);
    uint64_t dummy = 0;
    StepTaskT<int> rd = dcacheReadChecked(owner, state_ptr, 0x20u, REQUEST_TYPE_READ, state_ptr->init_pc, 0x0fu, &dummy, trace);
    int rd_ok = 0;
    CO_AWAIT_OWNED_VAL(rd_ok, owner, rd);
    if (!rd_ok) co_return 0;
    co_return 1;
  }
  const uint8_t addr_space = effectiveMemAddrSpace(state_ptr, op3, asi);
  uint32_t low2 = (eff_addr & 0x3u);
  uint8_t word_mask = (low2 == 0u) ? 0x8u : ((low2 == 1u) ? 0x4u : ((low2 == 2u) ? 0x2u : 0x1u));
  uint8_t dword_mask = ((eff_addr & 0x4u) ? word_mask : (uint8_t) (word_mask << 4));

  uint64_t rd64 = 0;
  StepTaskT<int> rd = dcacheReadChecked(owner,
                                        state_ptr,
                                        addr_space,
                                        (uint8_t) (REQUEST_TYPE_READ | SET_LOCK_FLAG),
                                        eff_addr,
                                        dword_mask,
                                        &rd64,
                                        trace);
  int rd_ok = 0;
  CO_AWAIT_OWNED_VAL(rd_ok, owner, rd);
  if (!rd_ok) {
    setDataAccessTrap(state_ptr);
    co_return 1;
  }

  uint32_t w32 = (uint32_t) ((eff_addr & 0x4u) ? (rd64 & 0xffffffffull) : (rd64 >> 32));
  uint32_t shift = (uint32_t) ((3u - low2) * 8u);
  uint32_t old_b = (w32 >> shift) & 0xffu;

  uint32_t wdata32 = 0xffffffffu;
  uint64_t w64 = ((eff_addr & 0x4u) ? (uint64_t) wdata32 : (((uint64_t) wdata32) << 32));
  StepTaskT<int> wr = dcacheWriteChecked(owner, state_ptr, addr_space, eff_addr, dword_mask, w64, trace);
  int wr_ok = 0;
  CO_AWAIT_OWNED_VAL(wr_ok, owner, wr);
  if (!wr_ok) {
    setDataAccessTrap(state_ptr);
    co_return 1;
  }
  recordStoreUpdate(state_ptr, addr_space, eff_addr, false, word_mask, 0, 0xffffffffu);

  *result_l = old_b;
  *flags = setBit8(*flags, _NEED_WRITE_BACK_, 1);
  co_return 1;
}

StepTaskT<int> executeSwap(CoroutineOwner* owner,
                           ThreadState* state_ptr,
                           uint8_t asi,
                           uint32_t operand1_0,
                           uint32_t operand2_0,
                           uint32_t data0,
                           uint32_t* result_l,
                           uint8_t* flags,
                           int trace)
{
  uint32_t eff_addr = (operand1_0 + operand2_0);
  const uint32_t op3 = (state_ptr->instruction >> 19) & 0x3fu;
  const uint8_t s = (uint8_t) getBit32(state_ptr->status_reg.psr, 7);
  const uint8_t i = (uint8_t) getBit32(state_ptr->instruction, 13);
  const bool is_alternate = (op3 == 0x1fu);
  const uint8_t trap_asi = is_alternate ? (uint8_t) (asi & 0x3fu) : (s ? 11u : 10u);
  const uint8_t addr_space = effectiveMemAddrSpace(state_ptr, op3, asi);

  bool is_privileged_trap = false;
  bool is_illegal_trap = false;
  bool is_alignment_trap = false;

  if (is_alternate && !s) {
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _PRIVILEGED_INSTRUCTION_, 1);
    recordStoreUpdate(state_ptr, trap_asi, eff_addr, false, 0x0u, 0, 0);
    is_privileged_trap = true;
  }
  if (is_alternate && i) {
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _ILLEGAL_INSTRUCTION_, 1);
    recordStoreUpdate(state_ptr, trap_asi, eff_addr, false, 0x0u, 0, 0);
    is_illegal_trap = true;
  }
  if ((eff_addr & 0x3u) != 0u) {
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _MEM_ADDRESS_NOT_ALIGNED_, 1);
    recordStoreUpdate(state_ptr, trap_asi, eff_addr, false, 0x0u, 0, 0);
    is_alignment_trap = true;
  }

  uint32_t old_w = 0;
  uint8_t dword_mask = ((eff_addr & 0x4u) ? 0x0fu : 0xf0u);
  if (!is_privileged_trap && !is_illegal_trap && !is_alignment_trap) {
    // Legacy swap path serializes lock users via block flags.
    testAndSetBlockLdstFlags(state_ptr, 0, 1);
    uint64_t rd64 = 0;
    StepTaskT<int> rd = dcacheRequestMae(owner,
                                         state_ptr,
                                         addr_space,
                                         eff_addr,
                                         (uint8_t) (REQUEST_TYPE_READ | SET_LOCK_FLAG),
                                         dword_mask,
                                         0,
                                         &rd64);
    int rd_ok = 0;
    CO_AWAIT_OWNED_VAL(rd_ok, owner, rd);
    if (!rd_ok) {
      state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
      state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _DATA_ACCESS_EXCEPTION_, 1);
    } else {
      old_w = (uint32_t) ((eff_addr & 0x4u) ? (rd64 & 0xffffffffull) : (rd64 >> 32));
    }
  }

  if (getBit32(state_ptr->trap_vector, _TRAP_)) {
    uint64_t dummy = 0;
    StepTaskT<int> unlock = dcacheReadChecked(owner, state_ptr, 0x20u, REQUEST_TYPE_READ, state_ptr->init_pc, 0x0fu, &dummy, trace);
    int unlock_ok = 0;
    CO_AWAIT_OWNED_VAL(unlock_ok, owner, unlock);
    if (!unlock_ok) co_return 0;
  } else {
    uint64_t w64 = ((eff_addr & 0x4u) ? (uint64_t) data0 : (((uint64_t) data0) << 32));
    StepTaskT<int> wr = dcacheRequestMae(owner,
                                         state_ptr,
                                         addr_space,
                                         eff_addr,
                                         REQUEST_TYPE_WRITE,
                                         dword_mask,
                                         w64,
                                         nullptr);
    int wr_ok = 0;
    CO_AWAIT_OWNED_VAL(wr_ok, owner, wr);
    if (!wr_ok) {
      state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
      state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _DATA_ACCESS_EXCEPTION_, 1);
    } else {
      recordStoreUpdate(state_ptr, addr_space, eff_addr, false, 0x0fu, 0, data0);
    }
  }
  // Legacy behavior: clear word lock flag even when trap path was taken.
  setPbBlockLdstWord(state_ptr, 0);

  *result_l = old_w;
  *flags = setBit8(*flags, _NEED_WRITE_BACK_, 1);
  co_return 1;
}

StepTaskT<int> executeCswap(CoroutineOwner* owner,
                            ThreadState* state_ptr,
                            uint8_t i,
                            uint8_t asi,
                            uint32_t operand1_0,
                            uint32_t operand2_0,
                            uint32_t data0,
                            uint32_t* result_l,
                            uint8_t* flags,
                            int trace)
{
  const uint32_t op3 = (state_ptr->instruction >> 19) & 0x3fu;
  const uint8_t addr_space = effectiveMemAddrSpace(state_ptr, op3, asi);
  uint32_t eff_addr = operand1_0;
  if ((op3 == 0x3fu) && i) {
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _ILLEGAL_INSTRUCTION_, 1);
    co_return 1;
  }
  if ((eff_addr & 0x3u) != 0u) {
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _MEM_ADDRESS_NOT_ALIGNED_, 1);
    co_return 1;
  }

  uint8_t dword_mask = ((eff_addr & 0x4u) ? 0x0fu : 0xf0u);
  uint64_t rd64 = 0;
  StepTaskT<int> rd = dcacheReadChecked(owner,
                                        state_ptr,
                                        addr_space,
                                        (uint8_t) (REQUEST_TYPE_READ | SET_LOCK_FLAG),
                                        eff_addr,
                                        dword_mask,
                                        &rd64,
                                        trace);
  int rd_ok = 0;
  CO_AWAIT_OWNED_VAL(rd_ok, owner, rd);
  if (!rd_ok) {
    setDataAccessTrap(state_ptr);
    co_return 1;
  }

  uint32_t read_word = (uint32_t) ((eff_addr & 0x4u) ? (rd64 & 0xffffffffull) : (rd64 >> 32));
  uint32_t write_word = (read_word == operand2_0) ? data0 : read_word;
  uint64_t w64 = ((eff_addr & 0x4u) ? (uint64_t) write_word : (((uint64_t) write_word) << 32));
  StepTaskT<int> wr = dcacheWriteChecked(owner, state_ptr, addr_space, eff_addr, dword_mask, w64, trace);
  int wr_ok = 0;
  CO_AWAIT_OWNED_VAL(wr_ok, owner, wr);
  if (!wr_ok) {
    setDataAccessTrap(state_ptr);
    co_return 1;
  }

  *result_l = (read_word == operand2_0) ? read_word : data0;
  *flags = setBit8(*flags, _NEED_WRITE_BACK_, 1);
  co_return 1;
}

StepTaskT<int> executeStore(CoroutineOwner* owner,
                            ThreadState* state_ptr,
                            uint8_t asi,
                            uint32_t operand1_0,
                            uint32_t operand2_0,
                            uint32_t data0,
                            uint32_t data1,
                            int trace)
{
  const uint32_t op3 = (state_ptr->instruction >> 19) & 0x3fu;
  const uint8_t addr_space = effectiveMemAddrSpace(state_ptr, op3, asi);
  const uint8_t s = (uint8_t) getBit32(state_ptr->status_reg.psr, 7);
  const uint8_t ef = (uint8_t) getBit32(state_ptr->status_reg.psr, 12);
  const uint8_t ec = (uint8_t) getBit32(state_ptr->status_reg.psr, 13);
  uint32_t eff_addr = (operand1_0 + operand2_0);

  // Match legacy executeStore trap gating before memory side effects.
  const bool is_alternate = ((op3 >= 0x14u) && (op3 <= 0x17u));
  const bool is_privileged = ((is_alternate || (op3 == 0x26u) || (op3 == 0x36u)) && (!s));
  if (is_privileged) {
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _PRIVILEGED_INSTRUCTION_, 1);
    co_return 1;
  }

  const bool is_fp_trap = (((op3 == 0x24u) || (op3 == 0x25u) || (op3 == 0x26u) || (op3 == 0x27u)) &&
                           ((!ef) || !getBpFPUPresent(state_ptr)));
  if (is_fp_trap) {
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _FP_DISABLED_, 1);
    co_return 1;
  }

  const bool is_cp_trap = (((op3 == 0x34u) || (op3 == 0x35u) || (op3 == 0x36u) || (op3 == 0x37u)) &&
                           ((!ec) || !getBpCPPresent(state_ptr)));
  if (is_cp_trap) {
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _CP_DISABLED_, 1);
    co_return 1;
  }

  const bool misaligned_hw = (((op3 == 0x06u) || (op3 == 0x16u)) && ((eff_addr & 0x1u) != 0u));
  const bool misaligned_fw = (((op3 == 0x04u) || (op3 == 0x14u) || (op3 == 0x24u) || (op3 == 0x25u) ||
                               (op3 == 0x34u) || (op3 == 0x35u)) &&
                              ((eff_addr & 0x3u) != 0u));
  const bool misaligned_dw = ((((op3 & 0x0fu) == 0x07u) || (op3 == 0x26u) || (op3 == 0x36u)) &&
                              ((eff_addr & 0x7u) != 0u));
  if (misaligned_hw || misaligned_fw || misaligned_dw) {
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _MEM_ADDRESS_NOT_ALIGNED_, 1);
    co_return 1;
  }

  if ((op3 == 0x04u) || (op3 == 0x14u) || (op3 == 0x24u) || (op3 == 0x34u)) {
    uint8_t byte_mask = ((eff_addr & 0x4u) ? 0x0fu : 0xf0u);
    uint64_t w32 = ((uint64_t) data0) & 0xffffffffull;
    uint64_t bus_wdata = ((eff_addr & 0x4u) ? w32 : (w32 << 32));
    StepTaskT<int> wr = dcacheWriteChecked(owner, state_ptr, addr_space, eff_addr, byte_mask, bus_wdata, trace);
    int wr_ok = 0;
    CO_AWAIT_OWNED_VAL(wr_ok, owner, wr);
    if (!wr_ok) {
      setDataAccessTrap(state_ptr);
      co_return 1;
    }
    recordStoreUpdate(state_ptr, addr_space, eff_addr, false, 0x0fu, 0, data0);
    co_return 1;
  }

  if ((op3 == 0x25u) || (op3 == 0x35u)) {
    uint32_t src = (op3 == 0x25u) ? state_ptr->status_reg.fsr : state_ptr->status_reg.csr;
    uint8_t byte_mask = ((eff_addr & 0x4u) ? 0x0fu : 0xf0u);
    uint64_t w32 = ((uint64_t) src) & 0xffffffffull;
    uint64_t bus_wdata = ((eff_addr & 0x4u) ? w32 : (w32 << 32));
    StepTaskT<int> wr = dcacheWriteChecked(owner, state_ptr, addr_space, eff_addr, byte_mask, bus_wdata, trace);
    int wr_ok = 0;
    CO_AWAIT_OWNED_VAL(wr_ok, owner, wr);
    if (!wr_ok) {
      setDataAccessTrap(state_ptr);
      co_return 1;
    }
    recordStoreUpdate(state_ptr, addr_space, eff_addr, false, 0x0fu, 0, src);
    co_return 1;
  }

  if ((op3 == 0x05u) || (op3 == 0x15u)) {
    uint32_t byte_sel = (eff_addr & 0x3u);
    uint8_t word_mask = 0;
    uint32_t word_data = data0;
    if (byte_sel == 0u) { word_mask = 0x8u; word_data = (data0 << 24); }
    else if (byte_sel == 1u) { word_mask = 0x4u; word_data = (data0 << 16); }
    else if (byte_sel == 2u) { word_mask = 0x2u; word_data = (data0 << 8); }
    else { word_mask = 0x1u; }
    uint8_t dword_mask = ((eff_addr & 0x4u) ? word_mask : (uint8_t) (word_mask << 4));
    uint64_t w32 = ((uint64_t) word_data) & 0xffffffffull;
    uint64_t bus_wdata = ((eff_addr & 0x4u) ? w32 : (w32 << 32));
    StepTaskT<int> wr = dcacheWriteChecked(owner, state_ptr, addr_space, eff_addr, dword_mask, bus_wdata, trace);
    int wr_ok = 0;
    CO_AWAIT_OWNED_VAL(wr_ok, owner, wr);
    if (!wr_ok) {
      setDataAccessTrap(state_ptr);
      co_return 1;
    }
    recordStoreUpdate(state_ptr, addr_space, eff_addr, false, word_mask, 0, word_data);
    co_return 1;
  }

  if ((op3 == 0x06u) || (op3 == 0x16u)) {
    uint32_t hw_sel = (eff_addr & 0x2u);
    uint8_t word_mask = 0;
    uint32_t word_data = data0;
    if (hw_sel == 0u) { word_mask = 0xCu; word_data = (data0 << 16); }
    else { word_mask = 0x3u; }
    uint8_t dword_mask = ((eff_addr & 0x4u) ? word_mask : (uint8_t) (word_mask << 4));
    uint64_t w32 = ((uint64_t) word_data) & 0xffffffffull;
    uint64_t bus_wdata = ((eff_addr & 0x4u) ? w32 : (w32 << 32));
    StepTaskT<int> wr = dcacheWriteChecked(owner, state_ptr, addr_space, eff_addr, dword_mask, bus_wdata, trace);
    int wr_ok = 0;
    CO_AWAIT_OWNED_VAL(wr_ok, owner, wr);
    if (!wr_ok) {
      setDataAccessTrap(state_ptr);
      co_return 1;
    }
    recordStoreUpdate(state_ptr, addr_space, eff_addr, false, word_mask, 0, word_data);
    co_return 1;
  }

  if ((op3 & 0x0fu) == 0x07u || (op3 == 0x26u) || (op3 == 0x36u)) {
    uint64_t bus_wdata = (((uint64_t) data0) << 32) | ((uint64_t) data1);
    StepTaskT<int> wr = dcacheWriteChecked(owner, state_ptr, addr_space, eff_addr, 0xff, bus_wdata, trace);
    int wr_ok = 0;
    CO_AWAIT_OWNED_VAL(wr_ok, owner, wr);
    if (!wr_ok) {
      setDataAccessTrap(state_ptr);
      co_return 1;
    }
    recordStoreUpdate(state_ptr, addr_space, eff_addr, true, 0xff, data0, data1);

    co_return 1;
  }

  co_return -1;
}

StepTaskT<int> executeLoad(CoroutineOwner* owner,
                           ThreadState* state_ptr,
                           uint8_t asi,
                           uint8_t rd,
                           uint32_t operand1_0,
                           uint32_t operand2_0,
                           uint32_t* result_h,
                           uint32_t* result_l,
                           uint8_t* flags,
                           int trace)
{
  const uint32_t op3 = (state_ptr->instruction >> 19) & 0x3fu;
  const uint8_t addr_space = effectiveMemAddrSpace(state_ptr, op3, asi);
  const uint8_t s = (uint8_t) getBit32(state_ptr->status_reg.psr, 7);
  const uint8_t ef = (uint8_t) getBit32(state_ptr->status_reg.psr, 12);
  const uint8_t ec = (uint8_t) getBit32(state_ptr->status_reg.psr, 13);
  uint32_t eff_addr = (operand1_0 + operand2_0);
  uint64_t rd64 = 0;

  const bool is_alternate = ((op3 & 0x30u) == 0x10u);
  const bool is_privileged = (is_alternate && !s);
  if (is_privileged) {
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _PRIVILEGED_INSTRUCTION_, 1);
    co_return 1;
  }

  const bool is_fp_load = ((op3 == 0x20u) || (op3 == 0x21u) || (op3 == 0x23u));
  if (is_fp_load && ((!ef) || !getBpFPUPresent(state_ptr))) {
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _FP_DISABLED_, 1);
    co_return 1;
  }

  const bool is_cp_load = ((op3 == 0x30u) || (op3 == 0x31u) || (op3 == 0x33u));
  if (is_cp_load && ((!ec) || !getBpCPPresent(state_ptr))) {
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _CP_DISABLED_, 1);
    co_return 1;
  }

  const bool misaligned_dw = ((op3 == 0x03u) || (op3 == 0x13u) || (op3 == 0x23u) || (op3 == 0x33u))
                             && ((eff_addr & 0x7u) != 0u);
  const bool misaligned_fw = ((op3 == 0x00u) || (op3 == 0x10u) || (op3 == 0x20u) || (op3 == 0x21u) ||
                              (op3 == 0x30u) || (op3 == 0x31u))
                             && ((eff_addr & 0x3u) != 0u);
  const bool misaligned_hw = ((op3 == 0x02u) || (op3 == 0x12u) || (op3 == 0x0au) || (op3 == 0x1au))
                             && ((eff_addr & 0x1u) != 0u);
  if (misaligned_dw || misaligned_fw || misaligned_hw) {
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _MEM_ADDRESS_NOT_ALIGNED_, 1);
    co_return 1;
  }

  if ((op3 == 0x23u) && (rd & 0x1u)) {
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _TRAP_, 1);
    state_ptr->trap_vector = setBit32(state_ptr->trap_vector, _INVALID_FP_REGISTER_, 1);
    state_ptr->status_reg.fsr = setSlice32(state_ptr->status_reg.fsr, 16, 14, 6);
    co_return 1;
  }

  if (op3 == 0x23u || op3 == 0x03u || op3 == 0x13u || op3 == 0x33u) {
    StepTaskT<int> rt = dcacheReadChecked(owner,
                                          state_ptr,
                                          addr_space,
                                          REQUEST_TYPE_READ,
                                          eff_addr,
                                          0xff,
                                          &rd64,
                                          trace);
    int rt_ok = 0;
    CO_AWAIT_OWNED_VAL(rt_ok, owner, rt);
    if (!rt_ok) {
      setDataAccessTrap(state_ptr);
      co_return 1;
    }
    *result_h = (uint32_t) (rd64 >> 32);
    *result_l = (uint32_t) (rd64 & 0xffffffffu);
    if (op3 == 0x23u) *flags = setBit8(setBit8(setBit8(*flags, _FLOAT_INSTRUCTION_, 1), _DOUBLE_RESULT_, 1), _NEED_WRITE_BACK_, 1);
    else if ((op3 == 0x03u) || (op3 == 0x13u)) *flags = setBit8(setBit8(*flags, _DOUBLE_RESULT_, 1), _NEED_WRITE_BACK_, 1);
    else if (op3 == 0x33u) *flags = setBit8(*flags, _DOUBLE_RESULT_, 1);
    co_return 1;
  }

  if ((op3 == 0x00u) || (op3 == 0x10u) || (op3 == 0x20u) || (op3 == 0x21u) || (op3 == 0x30u) || (op3 == 0x31u)) {
    uint8_t mask = ((eff_addr & 0x4u) ? 0x0fu : 0xf0u);
    StepTaskT<int> rt = dcacheReadChecked(owner,
                                          state_ptr,
                                          addr_space,
                                          REQUEST_TYPE_READ,
                                          eff_addr,
                                          mask,
                                          &rd64,
                                          trace);
    int rt_ok = 0;
    CO_AWAIT_OWNED_VAL(rt_ok, owner, rt);
    if (!rt_ok) {
      setDataAccessTrap(state_ptr);
      co_return 1;
    }
    uint32_t word0 = (uint32_t) ((eff_addr & 0x4u) ? (rd64 & 0xffffffffull) : (rd64 >> 32));
    if (op3 == 0x21u) state_ptr->status_reg.fsr = word0;
    if (op3 == 0x31u) state_ptr->status_reg.csr = word0;
    *result_l = word0;
    if (op3 == 0x20u) *flags = setBit8(*flags, _FLOAT_INSTRUCTION_, 1);
    if ((op3 != 0x30u) && (op3 != 0x31u)) *flags = setBit8(*flags, _NEED_WRITE_BACK_, 1);
    co_return 1;
  }

  if ((op3 == 0x01u) || (op3 == 0x11u) || (op3 == 0x02u) || (op3 == 0x12u) ||
      (op3 == 0x09u) || (op3 == 0x19u) || (op3 == 0x0au) || (op3 == 0x1au)) {
    const uint32_t low2 = (eff_addr & 0x3u);
    const bool is_byte = ((op3 == 0x01u) || (op3 == 0x11u) || (op3 == 0x09u) || (op3 == 0x19u));
    const bool is_signed = ((op3 == 0x09u) || (op3 == 0x19u) || (op3 == 0x0au) || (op3 == 0x1au));
    uint8_t byte_mask = 0;
    if (is_byte) byte_mask = (low2 == 0u) ? 0x8u : ((low2 == 1u) ? 0x4u : ((low2 == 2u) ? 0x2u : 0x1u));
    else byte_mask = ((low2 & 0x2u) ? 0x3u : 0xcu);
    uint8_t dword_mask = ((eff_addr & 0x4u) ? byte_mask : (uint8_t) (byte_mask << 4));
    StepTaskT<int> rt = dcacheReadChecked(owner,
                                          state_ptr,
                                          addr_space,
                                          REQUEST_TYPE_READ,
                                          eff_addr,
                                          dword_mask,
                                          &rd64,
                                          trace);
    int rt_ok = 0;
    CO_AWAIT_OWNED_VAL(rt_ok, owner, rt);
    if (!rt_ok) {
      setDataAccessTrap(state_ptr);
      co_return 1;
    }
    uint32_t w32 = (uint32_t) ((eff_addr & 0x4u) ? (rd64 & 0xffffffffull) : (rd64 >> 32));
    uint32_t loaded = 0;
    if (is_byte) {
      uint32_t shift = (uint32_t) ((3u - low2) * 8u);
      uint32_t b = (w32 >> shift) & 0xffu;
      loaded = is_signed ? (uint32_t) ((int32_t) (int8_t) b) : b;
    } else {
      uint32_t shift = ((low2 & 0x2u) ? 0u : 16u);
      uint32_t h = (w32 >> shift) & 0xffffu;
      loaded = is_signed ? (uint32_t) ((int32_t) (int16_t) h) : h;
    }
    *result_l = loaded;
    *flags = setBit8(*flags, _NEED_WRITE_BACK_, 1);
    co_return 1;
  }

  co_return -1;
}

StepTaskT<int> executeStbar(CoroutineOwner* owner,
                            ThreadState* state_ptr,
                            int trace)
{
  StepTaskT<int> stbar_req = dcacheRequestMae(owner,
                                              state_ptr,
                                              0,
                                              0,
                                              REQUEST_TYPE_STBAR,
                                              0x0,
                                              0,
                                              nullptr);
  int stbar_ok = 0;
  CO_AWAIT_OWNED_VAL(stbar_ok, owner, stbar_req);
  if (!stbar_ok) {
    if (trace) {
      std::fprintf(stderr,
                   "STEP-TASK-STBAR-FAIL c%u t%u sim=%llu\n",
                   state_ptr->core_id, state_ptr->thread_id,
                   (unsigned long long) state_ptr->sitar_sim_time);
    }
    co_return 0;
  }

  state_ptr->store_barrier_pending = 1;
  state_ptr->reg_update_flags.store_active = 1;
  state_ptr->reg_update_flags.store_asi = 0x0;
  state_ptr->reg_update_flags.store_addr = 0x0;
  state_ptr->reg_update_flags.store_double_word = 0;
  state_ptr->reg_update_flags.store_byte_mask = 0x0;
  state_ptr->reg_update_flags.store_word_low = 0;
  state_ptr->reg_update_flags.store_word_high = 0;
  co_return 1;
}

StepTaskT<int> executeFlush(CoroutineOwner* owner,
                            ThreadState* state_ptr,
                            uint32_t flush_addr,
                            int trace)
{
  StepTaskT<int> dflush_req = dcacheRequestMae(owner,
                                               state_ptr,
                                               ASI_FLUSH_I_D_CONTEXT,
                                               flush_addr,
                                               REQUEST_TYPE_WRITE,
                                               0x00,
                                               0,
                                               nullptr);
  int dflush_ok = 0;
  CO_AWAIT_OWNED_VAL(dflush_ok, owner, dflush_req);

  // Match legacy behavior: flush failure logs, but does not trap.
  if (!dflush_ok && trace) {
    std::fprintf(stderr,
                 "STEP-TASK-FLUSH-D-MAE c%u t%u sim=%llu addr=0x%08x\n",
                 state_ptr->core_id, state_ptr->thread_id,
                 (unsigned long long) state_ptr->sitar_sim_time,
                 flush_addr);
  }

  uint8_t flush_mae = 0;
  uint64_t dummy_ipair = 0;
  uint32_t mmu_fsr = 0;
  StepTaskT<void> itask =
      modular_thread_ports_enabled((int) state_ptr->core_id, (int) state_ptr->thread_id)
          ? modularCpuIcacheAccess(owner,
                                   (int) state_ptr->core_id,
                                   (int) state_ptr->thread_id,
                                   currentThreadContext(state_ptr),
                                   (uint8_t) (ASI_FLUSH_I_CONTEXT | 0x80u),
                                   flush_addr,
                                   REQUEST_TYPE_WRITE,
                                   0xff,
                                   &flush_mae,
                                   &dummy_ipair,
                                   &mmu_fsr)
          : cpuIcacheAccess(owner,
                            (int) state_ptr->core_id,
                            (int) state_ptr->thread_id,
                            currentThreadContext(state_ptr),
                            state_ptr->mmu_state,
                            state_ptr->icache,
                            (uint8_t) (ASI_FLUSH_I_CONTEXT | 0x80u),
                            flush_addr,
                            REQUEST_TYPE_WRITE,
                            0xff,
                            &flush_mae,
                            &dummy_ipair,
                            &mmu_fsr);
  CO_AWAIT_OWNED(owner, itask);
  if (flush_mae && trace) {
    std::fprintf(stderr,
                 "STEP-TASK-FLUSH-I-MAE c%u t%u sim=%llu addr=0x%08x fsr=0x%08x\n",
                 state_ptr->core_id, state_ptr->thread_id,
                 (unsigned long long) state_ptr->sitar_sim_time,
                 flush_addr, mmu_fsr);
  }

  // Keep instruction buffer coherent with flushed I-cache.
  if (state_ptr->i_buffer != nullptr) {
    clearInstructionDataBuffer(state_ptr->i_buffer);
  }

  state_ptr->reg_update_flags.store_active = 1;
  state_ptr->reg_update_flags.store_asi = ASI_FLUSH_I_D_CONTEXT;
  state_ptr->reg_update_flags.store_addr = flush_addr;
  state_ptr->reg_update_flags.store_double_word = 0;
  state_ptr->reg_update_flags.store_byte_mask = 0x0;
  state_ptr->reg_update_flags.store_word_low = 0;
  state_ptr->reg_update_flags.store_word_high = 0;
  co_return 1;
}

} // namespace

StepTaskT<int> executeInstruction(CoroutineOwner* owner,
                                  ThreadState* state_ptr,
                                  int trace,
                                  uint8_t i,
                                  uint8_t asi,
                                  uint8_t rd,
                                  uint8_t rs1,
                                  uint8_t vector_data_type,
                                  Opcode opcode,
                                  uint32_t operand2_0,
                                  uint32_t operand2_1,
                                  uint32_t operand1_0,
                                  uint32_t operand1_1,
                                  uint32_t data1,
                                  uint32_t data0,
                                  uint32_t* result_h,
                                  uint32_t* result_l,
                                  uint8_t* flags)
{
  uint8_t is_load = ((opcode >= _LDSB_) && (opcode <= _LDDA_));
  uint8_t is_store = ((opcode >= _STB_) && (opcode <= _STDA_));
  uint8_t is_atomic = ((opcode == _LDSTUB_) || (opcode == _LDSTUBA_));
  uint8_t is_swap = ((opcode == _SWAP_) || (opcode == _SWAPA_));
  uint8_t is_cswap = ((opcode == _CSWAP_) || (opcode == _CSWAPA_));

    if (is_load) {
      StepTaskT<int> load_task = executeLoad(owner,
                                             state_ptr,
                                             asi,
                                             rd,
                                             operand1_0,
                                             operand2_0,
                                             result_h,
                                             result_l,
                                             flags,
                                             trace);
      int load_ok = 0;
      CO_AWAIT_OWNED_VAL(load_ok, owner, load_task);
      if (load_ok == 0) {
        co_return 0;
      }
    } else if (is_store) {
      StepTaskT<int> store_task = executeStore(owner,
                                               state_ptr,
                                               asi,
                                               operand1_0,
                                               operand2_0,
                                               data0,
                                               data1,
                                               trace);
      int store_ok = 0;
      CO_AWAIT_OWNED_VAL(store_ok, owner, store_task);
      if (store_ok == 0) {
        co_return 0;
      }
    } else if (is_atomic) {
      StepTaskT<int> ldstub_task = executeLdstub(owner, state_ptr,
                                                 asi,
                                                 operand1_0, operand2_0, result_l, flags, trace);
      int ldstub_ok = 0;
      CO_AWAIT_OWNED_VAL(ldstub_ok, owner, ldstub_task);
      if (!ldstub_ok) {
        co_return 0;
      }
    } else if (is_swap) {
      StepTaskT<int> swap_task = executeSwap(owner, state_ptr,
                                             asi,
                                             operand1_0, operand2_0, data0, result_l, flags, trace);
      int swap_ok = 0;
      CO_AWAIT_OWNED_VAL(swap_ok, owner, swap_task);
      if (!swap_ok) {
        co_return 0;
      }
    } else if (is_cswap) {
      StepTaskT<int> cswap_task = executeCswap(owner, state_ptr, i,
                                               asi,
                                               operand1_0, operand2_0, data0, result_l, flags, trace);
      int cswap_ok = 0;
      CO_AWAIT_OWNED_VAL(cswap_ok, owner, cswap_task);
      if (!cswap_ok) {
        co_return 0;
      }
    } else {
    if (opcode == _STBAR_) {
      StepTaskT<int> stbar_task = executeStbar(owner, state_ptr, trace);
      int stbar_ok = 0;
      CO_AWAIT_OWNED_VAL(stbar_ok, owner, stbar_task);
      if (!stbar_ok) {
        co_return 0;
      }
      co_return 1;
    }
    if (opcode == _FLUSH_) {
      StepTaskT<int> flush_task = executeFlush(owner, state_ptr,
                                               (operand1_0 + operand2_0), trace);
      int flush_ok = 0;
      CO_AWAIT_OWNED_VAL(flush_ok, owner, flush_task);
      if (!flush_ok) {
        co_return 0;
      }
      co_return 1;
    }

    uint32_t old_pc = state_ptr->status_reg.pc;
    uint32_t old_npc = state_ptr->status_reg.npc;
    uint32_t old_psr = state_ptr->status_reg.psr;
    uint32_t old_wim = state_ptr->status_reg.wim;
    uint32_t trap_vector = state_ptr->trap_vector;
    StatusRegisters* status_reg = &(state_ptr->status_reg);
    RegisterFile* rf = state_ptr->register_file;

    uint8_t is_sethi = (opcode == _SETHI_);
    uint8_t is_nop = (opcode == _NOP_);
    uint8_t is_logical    = ((opcode >= _AND_) && (opcode <= _XNORcc_));
    uint8_t is_logical_64 = ((opcode >= _ANDD_) && (opcode <= _XNORDcc_));
    uint8_t is_shift     = ((opcode >= _SLL_) && (opcode <= _SRA_));
    uint8_t is_shift_64  = ((opcode >= _SLLD_) && (opcode <= _SRAD_));
    uint8_t is_add    = ((opcode >= _ADD_) && (opcode <= _ADDXcc_));
    uint8_t is_add_64 = ((opcode >= _ADDD_) && (opcode <= _ADDDcc_));
    uint8_t is_tadd   = ((opcode == _TADDcc_) || (opcode == _TADDccTV_));
    uint8_t is_sub    = ((opcode >= _SUB_) && (opcode <= _SUBXcc_));
    uint8_t is_sub_64 = ((opcode >= _SUBD_) && (opcode <= _SUBDcc_));
    uint8_t is_tsub = ((opcode == _TSUBcc_) || (opcode == _TSUBccTV_));
    uint8_t is_mul_step = (opcode == _MULScc_);
    uint8_t is_multiply    = ((opcode >= _UMUL_) && (opcode <= _SMULcc_));
    uint8_t is_multiply_64 = ((opcode >= _UMULD_) && (opcode <= _SMULDcc_));
    uint8_t is_divide = ((opcode >= _UDIV_) && (opcode <= _SDIVcc_));
    uint8_t is_divide_64 = ((opcode >= _UDIVD_) && (opcode <= _SDIVDcc_));
    uint8_t is_save = (opcode == _SAVE_);
    uint8_t is_restore = (opcode == _RESTORE_);
    uint8_t is_bicc = ((opcode >= _BA_) && (opcode <= _BVS_));
    uint8_t is_bfpcc = ((opcode >= _FBA_) && (opcode <= _FBO_));
    uint8_t is_bcpcc = ((opcode >= _CBA_) && (opcode <= _CB012_));
    uint8_t is_call = ((opcode == _CALL_));
    uint8_t is_jmpl = (opcode == _JMPL_);
    uint8_t is_rett = (opcode == _RETT_);
    uint8_t is_ticc = ((opcode >= _TA_) && (opcode <= _TVS_));
    uint8_t is_read_state_reg = ((opcode >= _RDY_) && (opcode <= _RDTBR_));
    uint8_t is_write_state_reg = ((opcode >= _WRY_) && (opcode <= _WRTBR_));
    uint8_t is_unimp = (opcode == _UNIMP_);
    uint8_t is_coprocessor_op = ((opcode >= _CPop1_) && (opcode<=_CPop2_));
    uint8_t is_iu_simd = ((opcode >= _VADDD8_) && (opcode <= _VSMULD32_));
    uint8_t is_byte_reduce = ((opcode == _ADDDREDUCE8_) ||
                              (opcode == _ANDDREDUCE8_) ||
                              (opcode == _ORDREDUCE8_) ||
                              (opcode == _XORDREDUCE8_));
    uint8_t is_halfword_reduce = ((opcode == _ADDDREDUCE16_) ||
                                  (opcode == _ANDDREDUCE16_) ||
                                  (opcode == _ORDREDUCE16_) ||
                                  (opcode == _XORDREDUCE16_));
    uint8_t is_byte_zpos = (opcode == _ZBYTEDPOS_);

    uint32_t tv = 0;
    uint32_t operand1 = operand1_0;
    uint32_t operand2 = operand2_0;

    if(is_logical)
      executeLogical(opcode, operand1, operand2, result_l, status_reg, &(state_ptr->reg_update_flags), flags);
    else if(is_logical_64)
      execute64BitLogical(opcode, operand1_0, operand1_1, operand2_0, operand2_1, result_h, result_l, status_reg, &(state_ptr->reg_update_flags), flags);
    else if(is_nop) executeNop();
    else if(is_sethi) executeSethi(operand1, result_l, flags);
    else if(is_shift)
      executeShift(opcode, operand1, operand2, result_l, flags);
    else if(is_shift_64)
      execute64BitShift(opcode, operand1_0, operand1_1, operand2, result_h, result_l, status_reg, &(state_ptr->reg_update_flags), flags);
    else if(is_add)
      executeAdd(opcode, operand1, operand2, result_l, status_reg, &(state_ptr->reg_update_flags), flags);
    else if(is_add_64)
      execute64BitAdd(opcode, operand1_0, operand1_1, operand2_0, operand2_1, result_h, result_l, status_reg, &(state_ptr->reg_update_flags), flags);
    else if(is_tadd)
      tv = executeTAdd(opcode, operand1, operand2, result_l, status_reg, &(state_ptr->reg_update_flags), trap_vector, flags);
    else if(is_sub)
      executeSub(opcode, operand1, operand2, result_l, status_reg, &(state_ptr->reg_update_flags), flags);
    else if(is_sub_64)
      execute64BitSub(opcode, operand1_0, operand1_1, operand2_0, operand2_1, result_h, result_l, status_reg, &(state_ptr->reg_update_flags), flags);
    else if(is_tsub)
      tv = executeTSub(opcode, operand1, operand2, result_l, status_reg, &(state_ptr->reg_update_flags), trap_vector, flags);
    else if(is_multiply)
      executeMul(opcode, operand1, operand2, result_l, status_reg, &(state_ptr->reg_update_flags), flags);
    else if(is_multiply_64)
      execute64BitMul(opcode, operand1_0, operand1_1, operand2_0, operand2_1, result_h, result_l, status_reg, &(state_ptr->reg_update_flags), flags);
    else if(is_mul_step)
      executeMulStep(operand1, operand2, result_l, status_reg, &(state_ptr->reg_update_flags), flags);
    else if(is_divide)
    {
      tv = executeDiv(opcode, operand1, operand2, result_l, status_reg, &(state_ptr->reg_update_flags), trap_vector, flags, state_ptr);
      if(global_enable_statistic_collection) {
        state_ptr->num_iu_divs_executed++;
        StepTask penalty = wait_penalty_cycles(owner, IU_DIV_PENALTY);
        CO_AWAIT_OWNED(owner, penalty);
      }
    }
    else if(is_divide_64)
      tv = execute64BitDiv(opcode, operand1_0, operand1_1, operand2_0, operand2_1, result_h, result_l, status_reg, &(state_ptr->reg_update_flags), trap_vector, flags, state_ptr);
    else if(is_save)
      tv = executeSave(operand1, operand2, result_l, status_reg, &(state_ptr->reg_update_flags), trap_vector, flags);
    else if(is_restore)
      tv = executeRestore(operand1, operand2, result_l, status_reg, &(state_ptr->reg_update_flags), trap_vector, flags);
    else if(is_bicc)
      tv = executeBicc(opcode, operand1, status_reg, trap_vector, *flags);
    else if(is_bfpcc)
      tv = executeBfpcc(state_ptr, opcode, operand1, status_reg, trap_vector, *flags);
    else if(is_bcpcc)
      tv = executeBcpcc(state_ptr, opcode, operand1, status_reg, trap_vector, *flags);
    else if(is_call)
      executeCall(rf, operand1, status_reg, &(state_ptr->reg_update_flags));
    else if(is_jmpl)
      tv = executeJumpAndLink(opcode, operand1, operand2, result_l, status_reg, trap_vector, flags);
    else if(is_rett)
      tv = executeRett(opcode, operand1, operand2, result_l, status_reg, &(state_ptr->reg_update_flags), trap_vector, &(state_ptr->mode));
    else if(is_ticc)
      tv = executeTicc(opcode, operand1, operand2, status_reg, &(state_ptr->reg_update_flags), trap_vector, &(state_ptr->ticc_trap_type));
    else if(is_read_state_reg)
      tv = executeReadStateReg(opcode, rs1, result_l, state_ptr, trap_vector, flags);
    else if(is_write_state_reg)
      tv = executeWriteStateReg(opcode, operand1, operand2, rd, status_reg, &(state_ptr->reg_update_flags), trap_vector);
    else if(is_unimp)
      tv = executeUnImplemented(trap_vector);
    else if(is_byte_reduce)
      execute64BitReduce8(opcode, operand1_0, operand1_1, operand2, result_l, status_reg, &(state_ptr->reg_update_flags), flags);
    else if(is_halfword_reduce)
      execute64BitReduce16(opcode, operand1_0, operand1_1, operand2, result_l, status_reg, &(state_ptr->reg_update_flags), flags);
    else if(is_byte_zpos)
      execute64BitZBytePos(opcode, operand1_0, operand1_1, operand2, result_l, status_reg, &(state_ptr->reg_update_flags), flags);
    else if(is_iu_simd)
      execute64BitVectorOp(opcode, vector_data_type, operand1_0, operand1_1, operand2_0, operand2_1, result_h, result_l, status_reg, &(state_ptr->reg_update_flags), flags);
    else if(is_coprocessor_op)
      tv = executeCoprocessor(trap_vector);

    if(opcode == _UNASSIGNED_) state_ptr->mode = _ERROR_MODE_;

    uint8_t is_cti = (is_bicc || is_bfpcc || is_bcpcc || is_call || is_jmpl || is_rett);
    if(is_cti)
    {
      const uint32_t bp_mispredicts_before = state_ptr->branch_predictor.mispredicts;
      uint32_t bp_nnpc;
      uint32_t bp_idx = 0;
      uint8_t br_taken = ((state_ptr->status_reg.pc + 4) != state_ptr->status_reg.npc);

      if(is_call) {
        pushIntoReturnAddressStack(&(state_ptr->return_address_stack), old_pc + 8);
      }

      uint8_t is_ret_or_retl = (is_jmpl && i && (operand2 == 8) && ((rs1 == 15) || (rs1 == 31)));
      if(is_ret_or_retl)
      {
        uint32_t pop_val = popFromReturnAddressStack(&(state_ptr->return_address_stack));
        if(pop_val & 0x1)
        {
          if((pop_val & (~0x1)) != state_ptr->status_reg.npc)
          {
            incrementRasMispredicts(&(state_ptr->return_address_stack));
            if(global_verbose_flag) {
              fprintf(stderr,"RAS: mispredict on 0x%x\n", pop_val & (~0x1));
            }
          }
        }
      }
      else
      {
        if(branchPrediction(&(state_ptr->branch_predictor), old_pc, state_ptr->status_reg.pc, &bp_idx, &bp_nnpc))
        {
          if(bp_nnpc != state_ptr->status_reg.npc) incrementMispredicts(&(state_ptr->branch_predictor));
          updateBranchPredictEntry(&(state_ptr->branch_predictor), bp_idx, br_taken, state_ptr->status_reg.npc);
        }
        else
        {
          if((state_ptr->status_reg.pc + 4) != state_ptr->status_reg.npc) incrementMispredicts(&(state_ptr->branch_predictor));
          addBranchPredictEntry(&(state_ptr->branch_predictor), br_taken, old_pc, state_ptr->status_reg.npc);
        }
      }
      if (state_ptr->branch_predictor.mispredicts != bp_mispredicts_before) {
        StepTask penalty = wait_penalty_cycles(owner, CTI_MISPREDICT_PENALTY);
        CO_AWAIT_OWNED(owner, penalty);
      }
    }

#ifdef SW
    if(state_ptr->mode == _ERROR_MODE_)
    {
      fprintf(stderr,"Entering ERROR mode\n" );
      fprintf(stderr,"encountered instruction with UNASSIGNED opcode \n");
      fprintf(stderr,"at PC = 0x%x, npc=0x%x, psr=0x%x, wim=0x%x\n",
              state_ptr->status_reg.pc,
              state_ptr->status_reg.npc,
              state_ptr->status_reg.psr,
              state_ptr->status_reg.wim);
      fprintf(stderr," instruction word = 0x%x\n", state_ptr->instruction);
    }
#endif
    state_ptr->trap_vector = tv;
  }

  co_return 1;
}
