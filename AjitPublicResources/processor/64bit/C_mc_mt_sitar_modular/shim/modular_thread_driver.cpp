#include "modular_thread_driver.h"

#include "ajit_memory_shim.h"
#include "ajit_thread_bridge.h"
#include "modular_packet_helpers.h"

#include "sitar_inport.h"
#include "sitar_module.h"
#include "sitar_outport.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _OPENMP
#include <omp.h>
#endif

extern "C" uint64_t ajit_sitar_sim_time;

namespace {

constexpr int kMaxSlots = 8;

struct ThreadPorts {
  bool enabled = false;

  void* ifetch_valid = nullptr;
  void* ifetch_kind = nullptr;
  void* ifetch_asi = nullptr;
  void* ifetch_context = nullptr;
  void* ifetch_addr = nullptr;
  void* ifetch_resp_valid = nullptr;
  void* ifetch_resp_mae = nullptr;
  void* ifetch_resp_data = nullptr;

  void* dcache_valid = nullptr;
  void* dcache_kind = nullptr;
  void* dcache_asi = nullptr;
  void* dcache_context = nullptr;
  void* dcache_addr = nullptr;
  void* dcache_data = nullptr;
  void* dcache_byte_mask = nullptr;
  void* dcache_resp_valid = nullptr;
  void* dcache_resp_mae = nullptr;
  void* dcache_resp_data = nullptr;
  void* irq_level = nullptr;
};

ThreadPorts g_ports[kMaxSlots];

int slot_for(int core_id, int thread_id)
{
  const int threads_per_core = ajit_thread_bridge_get_threads_per_core();
  const int tpc = (threads_per_core > 0) ? threads_per_core : 1;
  if (core_id < 0 || thread_id < 0 || thread_id >= tpc) {
    return -1;
  }
  const int slot = (core_id * tpc) + thread_id;
  return (slot >= 0 && slot < kMaxSlots) ? slot : -1;
}

bool real_thread_enabled()
{
  const char* env = std::getenv("AJIT_MODULAR_REAL_THREAD");
  return env != nullptr && env[0] != '\0' && std::strcmp(env, "0") != 0;
}

bool trace_adapter()
{
  const char* env = std::getenv("AJIT_MODULAR_TRACE_ADAPTER");
  return env != nullptr && env[0] != '\0' && std::strcmp(env, "0") != 0;
}

StepTask wait_for_phase_0(CoroutineOwner* owner)
{
  if ((ajit_sitar_sim_time & 0x1ull) == 0) {
    co_return;
  }
  co_await yield_point(owner);
}

StepTask wait_for_phase_1(CoroutineOwner* owner)
{
  if ((ajit_sitar_sim_time & 0x1ull) != 0) {
    co_return;
  }
  co_await yield_point(owner);
}

template<unsigned int Width>
sitar::outport<Width>* require_parent_outport(sitar::module* parent, const char* name)
{
  if (parent == nullptr) {
    std::fprintf(stderr, "MOD-THREAD missing parent TestBench while binding %s\n", name);
    std::abort();
  }
  auto it = parent->_outports.find(name);
  if ((it == parent->_outports.end()) || (it->second == nullptr) || (it->second->width() != Width)) {
    std::fprintf(stderr, "MOD-THREAD missing/wrong parent outport %s while binding TestBench boundary\n", name);
    std::abort();
  }
  return static_cast<sitar::outport<Width>*>(it->second);
}

template<unsigned int Width>
sitar::inport<Width>* require_parent_inport(sitar::module* parent, const char* name)
{
  if (parent == nullptr) {
    std::fprintf(stderr, "MOD-THREAD missing parent TestBench while binding %s\n", name);
    std::abort();
  }
  auto it = parent->_inports.find(name);
  if ((it == parent->_inports.end()) || (it->second == nullptr) || (it->second->width() != Width)) {
    std::fprintf(stderr, "MOD-THREAD missing/wrong parent inport %s while binding TestBench boundary\n", name);
    std::abort();
  }
  return static_cast<sitar::inport<Width>*>(it->second);
}

} // namespace

void ModularAjitThreadDriver::set_active(std::coroutine_handle<> h) noexcept
{
  active = h;
}

std::coroutine_handle<> ModularAjitThreadDriver::get_active() const noexcept
{
  return active;
}

void ModularAjitThreadDriver::bind_testbench_boundary_ports(sitar::module* thread_module)
{
  if (thread_module == nullptr) {
    std::fprintf(stderr, "MOD-THREAD missing module while binding TestBench boundary\n");
    std::abort();
  }
  sitar::module* tb = thread_module->parent();

  static_cast<sitar::inport<8>*>(irq_level)->setNet(require_parent_inport<8>(tb, "irq_level")->getNet());
  static_cast<sitar::outport<8>*>(coh_fill_kind)->setNet(require_parent_outport<8>(tb, "coh_fill_kind")->getNet());
  static_cast<sitar::outport<32>*>(coh_fill_pa_line)->setNet(require_parent_outport<32>(tb, "coh_fill_pa_line")->getNet());
  static_cast<sitar::outport<32>*>(coh_fill_va_line)->setNet(require_parent_outport<32>(tb, "coh_fill_va_line")->getNet());
  static_cast<sitar::inport<32>*>(coh_icache_inval)->setNet(require_parent_inport<32>(tb, "coh_icache_inval")->getNet());
  static_cast<sitar::inport<32>*>(coh_dcache_inval)->setNet(require_parent_inport<32>(tb, "coh_dcache_inval")->getNet());
}

void ModularAjitThreadDriver::register_ports()
{
  if (slot_id < 0 || slot_id >= kMaxSlots) {
    std::fprintf(stderr, "MOD-THREAD-DRIVER invalid slot=%d\n", slot_id);
    std::abort();
  }

  ThreadPorts& p = g_ports[slot_id];
  p.enabled = true;

  p.ifetch_valid = ifetch_valid;
  p.ifetch_kind = ifetch_kind;
  p.ifetch_asi = ifetch_asi;
  p.ifetch_context = ifetch_context;
  p.ifetch_addr = ifetch_addr;
  p.ifetch_resp_valid = ifetch_resp_valid;
  p.ifetch_resp_mae = ifetch_resp_mae;
  p.ifetch_resp_data = ifetch_resp_data;

  p.dcache_valid = dcache_valid;
  p.dcache_kind = dcache_kind;
  p.dcache_asi = dcache_asi;
  p.dcache_context = dcache_context;
  p.dcache_addr = dcache_addr;
  p.dcache_data = dcache_data;
  p.dcache_byte_mask = dcache_byte_mask;
  p.dcache_resp_valid = dcache_resp_valid;
  p.dcache_resp_mae = dcache_resp_mae;
  p.dcache_resp_data = dcache_resp_data;
  p.irq_level = irq_level;
  ajit_memory_shim_set_ports(slot_id,
                             nullptr,
                             nullptr,
                             nullptr,
                             nullptr,
                             nullptr,
                             nullptr,
                             irq_level,
                             coh_fill_kind,
                             coh_fill_pa_line,
                             coh_fill_va_line,
                             coh_icache_inval,
                             coh_dcache_inval);
  ajit_memory_shim_set_irq_port(slot_id, irq_level);
}

StepTaskT<int> ModularAjitThreadDriver::thread_step()
{
  StepTaskT<int> bridge_step = ajit_thread_bridge_step_task(this, slot_id, sim_time);
  bridge_step.set_owner(this);
  int rc = co_await bridge_step;
  co_return rc;
}

StepTask ModularAjitThreadDriver::driver()
{
  while (true) {
    if ((sim_time & 0x1ull) != 0) {
      co_await yield_point(this);
      continue;
    }

    StepTaskT<int> t = thread_step();
    t.set_owner(this);
    (void) co_await t;
    co_await yield_point(this);
  }
}

void ModularAjitThreadDriver::run(uint64_t simulation_time)
{
#ifdef _OPENMP
  int tid = omp_get_thread_num();
  if (owner_tid < 0) {
    owner_tid = tid;
  } else if (owner_tid != tid) {
    std::fprintf(stderr,
                 "ERROR: ModularAjitThreadDriver %d migrated OpenMP threads: was %d now %d\n",
                 slot_id,
                 owner_tid,
                 tid);
    std::abort();
  }
#endif

  sim_time = simulation_time;
  ajit_sitar_sim_time = simulation_time;
  ajit_shim_sample_irq(slot_id);

  if (!root) {
    root = driver();
    root.set_owner(this);
    set_active(root.h);
  }

  auto h = get_active();
  if (h && !h.done()) {
    h.resume();
  }
}

extern "C" int modular_thread_real_mode_enabled(void)
{
  return real_thread_enabled() ? 1 : 0;
}

extern "C" int modular_thread_ports_enabled(int core_id, int thread_id)
{
  const int slot = slot_for(core_id, thread_id);
  return (slot >= 0 && g_ports[slot].enabled) ? 1 : 0;
}

StepTaskT<void> modularCpuIcacheAccess(CoroutineOwner* owner,
                                       int core_id,
                                       int cpu_id,
                                       uint8_t context,
                                       uint8_t asi,
                                       uint32_t addr,
                                       uint8_t request_type,
                                       uint8_t byte_mask,
                                       uint8_t* mae,
                                       uint64_t* instr_pair,
                                       uint32_t* mmu_fsr)
{
  const int slot = slot_for(core_id, cpu_id);
  if (slot < 0 || !g_ports[slot].enabled) {
    if (mae != nullptr) *mae = 1;
    if (instr_pair != nullptr) *instr_pair = 0;
    if (mmu_fsr != nullptr) *mmu_fsr = 0;
    co_return;
  }

  ThreadPorts& p = g_ports[slot];

  const ModularThreadIcacheRequest request = {
      request_type,
      asi,
      context,
      addr,
  };
  uint8_t req_stage = 0;
  while (true) {
    StepTask p0 = modular_async_enabled() ? wait_for_phase_1(owner) : wait_for_phase_0(owner);
    CO_AWAIT_OWNED(owner, p0);
    if (modular_push_thread_icache_request_step(&req_stage,
                                                p.ifetch_valid,
                                                p.ifetch_kind,
                                                p.ifetch_asi,
                                                p.ifetch_context,
                                                p.ifetch_addr,
                                                request)) {
      break;
    }
    co_await yield_point(owner);
  }
  if (trace_adapter()) {
    std::fprintf(stderr,
                 "MOD-ADAPTER IFETCH issued c%d t%d sim=%llu kind=%u addr=0x%08x\n",
                 core_id,
                 cpu_id,
                 (unsigned long long) ajit_sitar_sim_time,
                 (unsigned) request_type,
                 addr);
  }

  ModularResponse response;
  uint8_t resp_stage = 0;
  while (true) {
    StepTask w1 = modular_async_enabled() ? wait_for_phase_0(owner) : wait_for_phase_1(owner);
    CO_AWAIT_OWNED(owner, w1);
    if (modular_pull_response_step(&resp_stage,
                                   p.ifetch_resp_valid,
                                   p.ifetch_resp_mae,
                                   p.ifetch_resp_data,
                                   &response)) {
      if (trace_adapter()) {
        std::fprintf(stderr,
                     "MOD-ADAPTER IFETCH response c%d t%d sim=%llu mae=%u data=0x%016llx\n",
                     core_id,
                     cpu_id,
                     (unsigned long long) ajit_sitar_sim_time,
                     (unsigned) response.mae,
                     (unsigned long long) response.data);
      }
      break;
    }
    co_await yield_point(owner);
  }

  if (mae != nullptr) *mae = response.mae;
  if (instr_pair != nullptr) *instr_pair = response.data;
  if (mmu_fsr != nullptr) *mmu_fsr = 0;
}

StepTaskT<void> modularCpuDcacheAccess(CoroutineOwner* owner,
                                       int core_id,
                                       int cpu_id,
                                       uint8_t context,
                                       uint8_t asi,
                                       uint32_t addr,
                                       uint8_t request_type,
                                       uint8_t byte_mask,
                                       uint64_t write_data,
                                       uint8_t* mae,
                                       uint64_t* read_data)
{
  const int slot = slot_for(core_id, cpu_id);
  if (slot < 0 || !g_ports[slot].enabled) {
    if (mae != nullptr) *mae = 1;
    if (read_data != nullptr) *read_data = 0;
    co_return;
  }

  ThreadPorts& p = g_ports[slot];

  const ModularThreadDcacheRequest request = {
      request_type,
      asi,
      context,
      addr,
      write_data,
      byte_mask,
  };
  uint8_t req_stage = 0;
  while (true) {
    StepTask p0 = modular_async_enabled() ? wait_for_phase_1(owner) : wait_for_phase_0(owner);
    CO_AWAIT_OWNED(owner, p0);
    if (modular_push_thread_dcache_request_step(&req_stage,
                                                p.dcache_valid,
                                                p.dcache_kind,
                                                p.dcache_asi,
                                                p.dcache_context,
                                                p.dcache_addr,
                                                p.dcache_data,
                                                p.dcache_byte_mask,
                                                request)) {
      break;
    }
    co_await yield_point(owner);
  }
  if (trace_adapter()) {
    std::fprintf(stderr,
                 "MOD-ADAPTER DCACHE issued c%d t%d sim=%llu kind=%u addr=0x%08x data=0x%016llx bm=0x%02x\n",
                 core_id,
                 cpu_id,
                 (unsigned long long) ajit_sitar_sim_time,
                 (unsigned) request_type,
                 addr,
                 (unsigned long long) write_data,
                 (unsigned) byte_mask);
  }

  ModularResponse response;
  uint8_t resp_stage = 0;
  while (true) {
    StepTask w1 = modular_async_enabled() ? wait_for_phase_0(owner) : wait_for_phase_1(owner);
    CO_AWAIT_OWNED(owner, w1);
    if (modular_pull_response_step(&resp_stage,
                                   p.dcache_resp_valid,
                                   p.dcache_resp_mae,
                                   p.dcache_resp_data,
                                   &response)) {
      if (trace_adapter()) {
        std::fprintf(stderr,
                     "MOD-ADAPTER DCACHE response c%d t%d sim=%llu mae=%u data=0x%016llx\n",
                     core_id,
                     cpu_id,
                     (unsigned long long) ajit_sitar_sim_time,
                     (unsigned) response.mae,
                     (unsigned long long) response.data);
      }
      break;
    }
    co_await yield_point(owner);
  }

  if (mae != nullptr) *mae = response.mae;
  if (read_data != nullptr) *read_data = response.data;
}
