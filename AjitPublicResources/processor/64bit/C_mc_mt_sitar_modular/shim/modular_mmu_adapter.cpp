#include "modular_mmu_adapter.h"

#include "ajit_memory_shim.h"
#include "ajit_thread_bridge.h"
#include "modular_port_helpers.h"

#include "sitar_inport.h"
#include "sitar_module.h"
#include "sitar_outport.h"

#include <cstdio>
#include <cstdlib>

extern "C" {
#include "ASI_values.h"
#include "MmuCommands.h"
#include "RequestTypeValues.h"
#include "TlbNew.h"
#include "rlut.h"

extern int global_enable_statistic_collection;
extern uint64_t ajit_sitar_sim_time;

uint8_t isTlbNewHit(MmuState* ms,
                    int thread_id,
                    uint32_t virt_addr,
                    uint32_t va_tag,
                    uint32_t* pte,
                    uint8_t* pte_level);
void writeTlbNewEntry(MmuState* ms,
                      int thread_id,
                      uint32_t virt_addr,
                      uint32_t va_tag,
                      uint32_t pte,
                      uint8_t pte_level);
uint8_t checkPageFaults(MmuState* ms,
                        int thread_id,
                        uint8_t pte_found,
                        uint32_t pte,
                        uint8_t pte_level,
                        uint8_t asi,
                        uint32_t virt_addr,
                        uint8_t request_type,
                        uint32_t* fsr_to_cache);
uint64_t constructPhysicalAddr(uint32_t pte, uint8_t pte_level, uint32_t virt_addr);
uint8_t isCacheable(uint32_t pte);
uint8_t isValidMmuRequest(uint8_t asi);
uint8_t isCacheRelatedRequest(uint8_t asi);
void updateFsrFar(MmuState* ms, int thread_id, uint32_t Fsr_val, uint32_t Far_val, uint8_t fault_class);
}

namespace {

constexpr uint8_t kIaccessFault = 1;
constexpr int kMaxModularCores = 4;
ModularMmuAdapter* g_mmu_adapters[kMaxModularCores] = {};

uint32_t parse_env_u32_min1(const char* name, uint32_t fallback)
{
  const char* raw = std::getenv(name);
  if (raw == nullptr || raw[0] == '\0') {
    return fallback;
  }
  char* end = nullptr;
  const unsigned long parsed = std::strtoul(raw, &end, 0);
  if (end == raw || parsed == 0) {
    return fallback;
  }
  return (uint32_t) parsed;
}

bool trace_adapter()
{
  const char* raw = std::getenv("AJIT_MODULAR_TRACE_ADAPTER");
  return raw != nullptr && raw[0] != '\0' && raw[0] != '0';
}

inline uint32_t get_slice32_u(uint32_t x, uint8_t hi, uint8_t lo)
{
  const uint32_t width = (uint32_t) (hi - lo + 1u);
  const uint32_t mask = (width >= 32u) ? 0xffffffffu : ((1u << width) - 1u);
  return (x >> lo) & mask;
}

inline uint64_t get_phy_addr_from_ptd(uint32_t ptd, uint32_t index)
{
  uint64_t phy_addr = (uint64_t) (ptd & ~0x3u);
  phy_addr <<= 4;
  phy_addr |= ((uint64_t) index << 2);
  return phy_addr;
}

inline int mmu_tid_index(int thread_id)
{
  return (thread_id & MMU_THREAD_MASK);
}

inline int source_index(uint8_t source)
{
  return (source == 0) ? 0 : 1;
}

inline uint32_t mmu_reg_index(uint32_t addr)
{
  return (addr >> 8) & 0xFFFFFFu;
}

inline int thread_slot_id(MmuState* ms, int thread_id)
{
  const int threads_per_core = ajit_thread_bridge_get_threads_per_core();
  const int tpc = (threads_per_core > 0) ? threads_per_core : 1;
  const int core = (ms != nullptr) ? (int) ms->core_id : 0;
  return (core * tpc) + thread_id;
}

StepTaskT<void> register_coherence_fill(CoroutineOwner* owner,
                                        MmuState* ms,
                                        int thread_id,
                                        uint8_t cache_kind,
                                        uint32_t pa_line_addr,
                                        uint32_t va_line_addr)
{
  const int slot = thread_slot_id(ms, thread_id);
  ajit_shim_coherence_fill_begin(slot, cache_kind, pa_line_addr, va_line_addr);
  while (!ajit_shim_coherence_fill(slot, ajit_sitar_sim_time)) {
    co_await yield_point(owner);
  }
  co_return;
}

inline uint32_t read_mmu_register_legacy(MmuState* ms, int thread_id, uint32_t addr)
{
  if (ms == nullptr) {
    return 0;
  }
  const int tid = mmu_tid_index(thread_id);
  switch (mmu_reg_index(addr)) {
    case 0:
      return ms->MmuControlRegister[tid];
    case 1:
      return ms->MmuContextTablePointerRegister[tid];
    case 2:
      return ms->MmuContextRegister[tid];
    case 3: {
      const uint32_t v = ms->MmuFaultStatusRegister[tid];
      ms->MmuFaultStatusRegister[tid] = 0;
      ms->Mmu_FSR_FAULT_CLASS[tid] = 0;
      return v;
    }
    case 4:
      return ms->MmuFaultAddressRegister[tid];
    default:
      return 0;
  }
}

inline bool mmu_enabled(MmuState* ms, int thread_id)
{
  if (ms == nullptr) {
    return false;
  }
  return (ms->MmuControlRegister[mmu_tid_index(thread_id)] & 0x1u) != 0;
}

inline bool mmu_cacheable_bit(MmuState* ms, int thread_id)
{
  if (ms == nullptr) {
    return false;
  }
  return (ms->MmuControlRegister[mmu_tid_index(thread_id)] & 0x100u) != 0;
}

inline void write_mmu_register_legacy(MmuState* ms,
                                      int thread_id,
                                      uint32_t addr,
                                      uint8_t byte_mask,
                                      uint64_t data64)
{
  if (ms == nullptr) {
    return;
  }
  const int tid = mmu_tid_index(thread_id);
  const uint32_t reg = mmu_reg_index(addr);
  uint32_t data32 = (uint32_t) data64;
  if (byte_mask == 0xf0u) {
    data32 = (uint32_t) (data64 >> 32);
  }

  auto write_all_threads_if_single_context = [&](volatile uint32_t* arr, uint32_t val) {
    if (!ms->multi_context) {
      for (int i = 0; i < MMU_MAX_NUMBER_OF_THREADS; ++i) {
        arr[i] = val;
      }
    } else {
      arr[tid] = val;
    }
  };

  switch (reg) {
    case 0:
      write_all_threads_if_single_context(ms->MmuControlRegister, data32);
      break;
    case 1:
      write_all_threads_if_single_context(ms->MmuContextTablePointerRegister, data32);
      break;
    case 2:
      write_all_threads_if_single_context(ms->MmuContextRegister, data32);
      break;
    default:
      break;
  }
}

template<unsigned int Width>
sitar::outport<Width>* require_parent_outport(sitar::module* parent, const char* name)
{
  if (parent == nullptr) {
    std::fprintf(stderr, "MOD-MMU missing parent TestBench while binding %s\n", name);
    std::abort();
  }
  auto it = parent->_outports.find(name);
  if ((it == parent->_outports.end()) || (it->second == nullptr) || (it->second->width() != Width)) {
    std::fprintf(stderr, "MOD-MMU missing/wrong parent outport %s while binding TestBench boundary\n", name);
    std::abort();
  }
  return static_cast<sitar::outport<Width>*>(it->second);
}

template<unsigned int Width>
sitar::inport<Width>* require_parent_inport(sitar::module* parent, const char* name)
{
  if (parent == nullptr) {
    std::fprintf(stderr, "MOD-MMU missing parent TestBench while binding %s\n", name);
    std::abort();
  }
  auto it = parent->_inports.find(name);
  if ((it == parent->_inports.end()) || (it->second == nullptr) || (it->second->width() != Width)) {
    std::fprintf(stderr, "MOD-MMU missing/wrong parent inport %s while binding TestBench boundary\n", name);
    std::abort();
  }
  return static_cast<sitar::inport<Width>*>(it->second);
}

} // namespace

void ModularMmuAdapter::init(int core)
{
  core_id = core;
  if (mmu_state == nullptr) {
    mmu_state = makeMmuState((uint32_t) core_id,
                             parse_env_u32_min1("AJIT_THREAD_TLB0_LOG_MEM_SIZE", 1),
                             parse_env_u32_min1("AJIT_THREAD_TLB0_LOG_SET_SIZE", 1),
                             parse_env_u32_min1("AJIT_THREAD_TLB1_LOG_MEM_SIZE", 3),
                             parse_env_u32_min1("AJIT_THREAD_TLB1_LOG_SET_SIZE", 3),
                             parse_env_u32_min1("AJIT_THREAD_TLB2_LOG_MEM_SIZE", 4),
                             parse_env_u32_min1("AJIT_THREAD_TLB2_LOG_SET_SIZE", 4),
                             parse_env_u32_min1("AJIT_THREAD_TLB3_LOG_MEM_SIZE", 6),
                             parse_env_u32_min1("AJIT_THREAD_TLB3_LOG_SET_SIZE", 3));
  }
  if (core_id >= 0 && core_id < kMaxModularCores) {
    g_mmu_adapters[core_id] = this;
  }
}

void ModularMmuAdapter::bind_testbench_bridge_ports(sitar::module* mmu_module)
{
  if (mmu_module == nullptr) {
    std::fprintf(stderr, "MOD-MMU missing module while binding TestBench boundary\n");
    std::abort();
  }
  sitar::module* tb = mmu_module->parent();

  static_cast<sitar::outport<1>*>(bridge_valid)->setNet(require_parent_outport<1>(tb, "active")->getNet());
  static_cast<sitar::outport<1>*>(bridge_write)->setNet(require_parent_outport<1>(tb, "write")->getNet());
  static_cast<sitar::outport<32>*>(bridge_addr)->setNet(require_parent_outport<32>(tb, "addr_out")->getNet());
  static_cast<sitar::outport<64>*>(bridge_data)->setNet(require_parent_outport<64>(tb, "data_out")->getNet());
  static_cast<sitar::outport<8>*>(bridge_byte_mask)->setNet(require_parent_outport<8>(tb, "byte_mask")->getNet());
  static_cast<sitar::inport<64>*>(bridge_resp_data)->setNet(require_parent_inport<64>(tb, "data_in")->getNet());
}

void ModularMmuAdapter::set_active(std::coroutine_handle<> h) noexcept
{
  active = h;
}

std::coroutine_handle<> ModularMmuAdapter::get_active() const noexcept
{
  return active;
}

bool ModularMmuAdapter::idle() const
{
  return !root || !root.h || root.h.done();
}

void ModularMmuAdapter::dump_summary() const
{
  uint64_t translated = 0;
  uint64_t tlb_hits = 0;
  uint64_t bypass = 0;
  uint64_t probe = 0;
  uint64_t flush = 0;
  uint64_t reg_reads = 0;
  uint64_t reg_writes = 0;
  if (mmu_state != nullptr) {
    for (int tid = 0; tid < MMU_MAX_NUMBER_OF_THREADS; ++tid) {
      translated += mmu_state->Num_Mmu_translated_accesses[tid];
      tlb_hits += mmu_state->Num_Mmu_TLB_hits[tid];
      bypass += mmu_state->Num_Mmu_bypass_accesses[tid];
      probe += mmu_state->Num_Mmu_probe_requests[tid];
      flush += mmu_state->Num_Mmu_flush_requests[tid];
      reg_reads += mmu_state->Num_Mmu_register_reads[tid];
      reg_writes += mmu_state->Num_Mmu_register_writes[tid];
    }
  }

  std::fprintf(stderr,
               "MOD-MMU-SUMMARY c%d req-if=%llu req-df=%llu line-if=%llu line-df=%llu "
               "rd-if=%llu rd-df=%llu wr-if=%llu wr-df=%llu wrnr-if=%llu wrnr-df=%llu "
               "wrfsr-if=%llu wrfsr-df=%llu bridge-if(a=%llu r=%llu w=%llu linew=%llu) "
               "bridge-df(a=%llu r=%llu w=%llu linew=%llu) trans=%llu tlb-hit=%llu "
               "bypass=%llu probe=%llu flush=%llu reg-r=%llu reg-w=%llu\n",
               core_id,
               (unsigned long long) client_requests[0],
               (unsigned long long) client_requests[1],
               (unsigned long long) read_line_requests[0],
               (unsigned long long) read_line_requests[1],
               (unsigned long long) read_dword_requests[0],
               (unsigned long long) read_dword_requests[1],
               (unsigned long long) write_dword_requests[0],
               (unsigned long long) write_dword_requests[1],
               (unsigned long long) write_dword_no_response_requests[0],
               (unsigned long long) write_dword_no_response_requests[1],
               (unsigned long long) write_fsr_requests[0],
               (unsigned long long) write_fsr_requests[1],
               (unsigned long long) bridge_accesses[0],
               (unsigned long long) bridge_reads[0],
               (unsigned long long) bridge_writes[0],
               (unsigned long long) bridge_line_words[0],
               (unsigned long long) bridge_accesses[1],
               (unsigned long long) bridge_reads[1],
               (unsigned long long) bridge_writes[1],
               (unsigned long long) bridge_line_words[1],
               (unsigned long long) translated,
               (unsigned long long) tlb_hits,
               (unsigned long long) bypass,
               (unsigned long long) probe,
               (unsigned long long) flush,
               (unsigned long long) reg_reads,
               (unsigned long long) reg_writes);
}

extern "C" void modular_mmu_dump_summary(void)
{
  for (int core_id = 0; core_id < kMaxModularCores; ++core_id) {
    if (g_mmu_adapters[core_id] != nullptr) {
      g_mmu_adapters[core_id]->dump_summary();
    }
  }
}

void ModularMmuAdapter::run(uint64_t simulation_time)
{
  sim_time = simulation_time;
  ajit_sitar_sim_time = simulation_time;
  auto h = get_active();
  if (h && !h.done()) {
    h.resume();
  }
}

bool ModularMmuAdapter::try_accept_client_request(int client_id)
{
  if (!idle() || client_id < 0 || client_id > 1) {
    return false;
  }

  void* valid = (client_id == 0) ? icache_valid : dcache_valid;
  void* source = (client_id == 0) ? icache_source : dcache_source;
  void* thread_id = (client_id == 0) ? icache_thread_id : dcache_thread_id;
  void* command = (client_id == 0) ? icache_command : dcache_command;
  void* kind = (client_id == 0) ? icache_kind : dcache_kind;
  void* asi = (client_id == 0) ? icache_asi : dcache_asi;
  void* context = (client_id == 0) ? icache_context : dcache_context;
  void* addr = (client_id == 0) ? icache_addr : dcache_addr;
  void* data = (client_id == 0) ? icache_data : dcache_data;
  void* byte_mask = (client_id == 0) ? icache_byte_mask : dcache_byte_mask;

  ModularMmuRequest& request = pending_client_request[client_id];
  if (!modular_pull_mmu_request_step(&client_pull_stage[client_id],
                                     valid,
                                     source,
                                     thread_id,
                                     command,
                                     kind,
                                     asi,
                                     context,
                                     addr,
                                     data,
                                     byte_mask,
                                     &request)) {
    return false;
  }

  active_client = client_id;
  if (trace_adapter()) {
    std::fprintf(stderr,
                 "MOD-MMU c%d accepted client=%d source=%u cmd=%u kind=%u asi=0x%02x va=0x%08x\n",
                 core_id,
                 active_client,
                 (unsigned) request.source,
                 (unsigned) request.mmu_command,
                 (unsigned) request.kind,
                 (unsigned) request.asi,
                 request.addr);
  }
  root = mmu_access_coroutine(request);
  root.set_owner(this);
  set_active(root.h);
  return true;
}

StepTask ModularMmuAdapter::wait_for_phase_0()
{
  while ((sim_time & 0x1ull) != 0) {
    co_await yield_point(this);
  }
}

StepTask ModularMmuAdapter::wait_for_phase_1()
{
  while ((sim_time & 0x1ull) == 0) {
    co_await yield_point(this);
  }
}

StepTaskT<ModularResponse> ModularMmuAdapter::bridge_access_over_nets(
    const ModularBridgeRequest& request)
{
  const uint8_t req_type = (uint8_t) (request.kind & 0x3fu);
  const int source = source_index(request.source);
  const bool write_request =
      (req_type == REQUEST_TYPE_WRITE) ||
      (req_type == REQUEST_TYPE_WRFSRFAR) ||
      (req_type == REQUEST_TYPE_CCU_CACHE_WRITE);
  const uint8_t final_stage = write_request ? 5 : 3;
  uint8_t req_stage = 0;
  bridge_accesses[source]++;
  if (write_request) {
    bridge_writes[source]++;
  } else {
    bridge_reads[source]++;
  }

  while (req_stage < final_stage) {
    StepTask p1 = wait_for_phase_1();
    CO_AWAIT_OWNED(this, p1);

    while (req_stage < final_stage) {
      bool ok = false;
      switch (req_stage) {
        case 0:
          ok = mod_push_bool(bridge_valid, true);
          break;
        case 1:
          ok = mod_push_bool(bridge_write, write_request);
          break;
        case 2:
          ok = mod_push_u32(bridge_addr, request.addr);
          break;
        case 3:
          ok = mod_push_u64(bridge_data, request.data);
          break;
        case 4:
          ok = mod_push_u8(bridge_byte_mask, request.byte_mask);
          break;
        default:
          ok = true;
          break;
      }
      if (!ok) {
        break;
      }
      ++req_stage;
    }

    if (req_stage < final_stage) {
      co_await yield_point(this);
    }
  }

  ModularResponse response;
  while (true) {
    StepTask p0 = wait_for_phase_0();
    CO_AWAIT_OWNED(this, p0);
    uint64_t response_data = 0;
    if (mod_pull_u64(bridge_resp_data, &response_data)) {
      response.mae = 0;
      response.data = response_data;
      co_return response;
    }
    co_await yield_point(this);
  }
}

StepTaskT<int> ModularMmuAdapter::read_pt_entry(uint8_t lock_mask,
                                                uint8_t thread_id,
                                                uint64_t physical_addr,
                                                uint32_t* pte)
{
  const ModularBridgeRequest bridge_request = {
      1,
      thread_id,
      (uint8_t) (REQUEST_TYPE_READ | lock_mask),
      (uint32_t) physical_addr,
      0,
      0,
  };
  StepTaskT<ModularResponse> bridge_task = bridge_access_over_nets(bridge_request);
  bridge_task.set_owner(this);
  ModularResponse bridge_response = co_await bridge_task;
  if (bridge_response.mae != 0) {
    co_return 0;
  }
  if ((physical_addr & 0x4ull) == 0ull) {
    *pte = (uint32_t) (bridge_response.data >> 32);
  } else {
    *pte = (uint32_t) bridge_response.data;
  }
  co_return 1;
}

StepTask ModularMmuAdapter::write_pt_entry(uint8_t lock_mask,
                                           uint8_t thread_id,
                                           uint64_t physical_addr,
                                           uint32_t pte)
{
  uint64_t data64 = pte;
  uint8_t byte_mask = 0x0f;
  if ((physical_addr & 0x4ull) == 0ull) {
    data64 <<= 32;
    byte_mask = 0xf0;
  }

  const ModularBridgeRequest bridge_request = {
      1,
      thread_id,
      (uint8_t) (REQUEST_TYPE_WRITE | lock_mask),
      (uint32_t) physical_addr,
      data64,
      byte_mask,
  };
  StepTaskT<ModularResponse> bridge_task = bridge_access_over_nets(bridge_request);
  bridge_task.set_owner(this);
  (void) co_await bridge_task;
}

StepTaskT<int> ModularMmuAdapter::walk_page_tables(uint8_t lock_mask,
                                                   uint8_t thread_id,
                                                   uint32_t virt_addr,
                                                   uint32_t* pte,
                                                   uint8_t* pte_level,
                                                   uint64_t* phy_addr_of_pte)
{
  const int tid = mmu_tid_index(thread_id);
  const uint32_t context_table_index = mmu_state->MmuContextRegister[tid];
  const uint32_t l1_index = get_slice32_u(virt_addr, 31, 24);
  const uint32_t l2_index = get_slice32_u(virt_addr, 23, 18);
  const uint32_t l3_index = get_slice32_u(virt_addr, 17, 12);

  uint64_t phy_addr =
      get_phy_addr_from_ptd(mmu_state->MmuContextTablePointerRegister[tid], context_table_index);
  uint32_t e = 0;
  int ok = 0;
  StepTaskT<int> r0 = read_pt_entry(lock_mask, thread_id, phy_addr, &e);
  r0.set_owner(this);
  ok = co_await r0;
  if (!ok) co_return 0;
  *pte_level = 0;

  if ((e & 0x3u) == 0x1u) {
    phy_addr = get_phy_addr_from_ptd(e, l1_index);
    StepTaskT<int> r1 = read_pt_entry(lock_mask, thread_id, phy_addr, &e);
    r1.set_owner(this);
    ok = co_await r1;
    if (!ok) co_return 0;
    *pte_level = 1;

    if ((e & 0x3u) == 0x1u) {
      phy_addr = get_phy_addr_from_ptd(e, l2_index);
      StepTaskT<int> r2 = read_pt_entry(lock_mask, thread_id, phy_addr, &e);
      r2.set_owner(this);
      ok = co_await r2;
      if (!ok) co_return 0;
      *pte_level = 2;

      if ((e & 0x3u) == 0x1u) {
        phy_addr = get_phy_addr_from_ptd(e, l3_index);
        StepTaskT<int> r3 = read_pt_entry(lock_mask, thread_id, phy_addr, &e);
        r3.set_owner(this);
        ok = co_await r3;
        if (!ok) co_return 0;
        *pte_level = 3;
      }
    }
  }

  *pte = e;
  *phy_addr_of_pte = phy_addr;
  co_return ((e & 0x3u) == 0x2u);
}

StepTaskT<int> ModularMmuAdapter::perform_probe(uint8_t lock_mask,
                                                uint8_t thread_id,
                                                uint32_t virt_addr,
                                                uint8_t asi,
                                                uint8_t request_type,
                                                uint32_t* pte)
{
  uint8_t pte_level = 0;
  uint32_t local_pte = 0;
  uint64_t phy_addr_of_pte = 0;
  const uint32_t va_tag = get_slice32_u(virt_addr, 31, 12);

  *pte = 0;
  if (TLB_ENABLED) {
    if (isTlbNewHit(mmu_state, thread_id, virt_addr, va_tag, &local_pte, &pte_level)) {
      *pte = local_pte;
      co_return 1;
    }
  }

  StepTaskT<int> walk = walk_page_tables(lock_mask,
                                         thread_id,
                                         virt_addr,
                                         &local_pte,
                                         &pte_level,
                                         &phy_addr_of_pte);
  walk.set_owner(this);
  const int pte_found = co_await walk;
  if (!pte_found) {
    uint32_t fsr_to_be_updated_after_probe = 0;
    checkPageFaults(mmu_state,
                    thread_id,
                    0,
                    local_pte,
                    pte_level,
                    asi,
                    virt_addr,
                    request_type,
                    &fsr_to_be_updated_after_probe);
    *pte = 0;
    co_return 0;
  }

  *pte = local_pte;
  co_return 1;
}

StepTask ModularMmuAdapter::push_client_response(int client_id, const ModularMmuResponse& response)
{
  void* valid = (client_id == 0) ? icache_resp_valid : dcache_resp_valid;
  void* mae = (client_id == 0) ? icache_resp_mae : dcache_resp_mae;
  void* data = (client_id == 0) ? icache_resp_data : dcache_resp_data;
  void* cacheable = (client_id == 0) ? icache_resp_cacheable : dcache_resp_cacheable;
  void* acc = (client_id == 0) ? icache_resp_acc : dcache_resp_acc;
  void* fsr = (client_id == 0) ? icache_resp_fsr : dcache_resp_fsr;
  void* synonym = (client_id == 0) ? icache_resp_synonym : dcache_resp_synonym;

  uint8_t resp_stage = 0;
  while (true) {
    StepTask p1 = wait_for_phase_1();
    CO_AWAIT_OWNED(this, p1);
    if (modular_push_mmu_response_step(&resp_stage,
                                       valid,
                                       mae,
                                       data,
                                       cacheable,
                                       acc,
                                       fsr,
                                       synonym,
                                       response, modular_async_enabled() ? ((client_id == 0) ? icache_resp_meta : dcache_resp_meta) : nullptr)) {
      co_return;
    }
    co_await yield_point(this);
  }
}

StepTaskT<ModularMmuResponse> ModularMmuAdapter::mmu_access_coroutine(ModularMmuRequest request)
{
  ModularMmuResponse response;
  uint64_t read_data = 0;
  const uint8_t req_type = (uint8_t) (request.kind & 0x3fu);
  const uint8_t lock_mask = (uint8_t) (request.kind & 0x40u);
  const uint8_t asi_7b = (uint8_t) (request.asi & 0x7fu);
  const uint8_t thread_id = (uint8_t) mmu_tid_index(request.thread_id);
  const uint8_t command = (request.mmu_command != 0) ? request.mmu_command : MMU_READ_DWORD;
  const int source = source_index((uint8_t) active_client);
  client_requests[source]++;
  switch (command) {
    case MMU_READ_LINE:
      read_line_requests[source]++;
      break;
    case MMU_READ_DWORD:
      read_dword_requests[source]++;
      break;
    case MMU_WRITE_DWORD:
      write_dword_requests[source]++;
      break;
    case MMU_WRITE_DWORD_NO_RESPONSE:
      write_dword_no_response_requests[source]++;
      break;
    case MMU_WRITE_FSR:
      write_fsr_requests[source]++;
      break;
    default:
      break;
  }

  // Only the async cache can create a physical-write certificate. Translation
  // and access permissions were validated before CPU acknowledgement. Never
  // retranslate a buffered store under a later context.
  if (command == MOD_MMU_WRITE_PHYSICAL) {
    if (!modular_async_enabled() || active_client != 1 ||
        req_type != REQUEST_TYPE_WRITE || lock_mask) {
      std::abort();
    }
    write_dword_no_response_requests[source]++;
    const ModularBridgeRequest physical = {
        request.source, thread_id, REQUEST_TYPE_WRITE,
        request.addr, request.data, request.byte_mask};
    auto task = bridge_access_over_nets(physical);
    task.set_owner(this);
    const ModularResponse r = co_await task;
    response.mae = r.mae;
    auto reply = push_client_response(active_client, response);
    CO_AWAIT_OWNED(this, reply);
    co_return response;
  }

  if (command == MMU_WRITE_FSR) {
    if ((mmu_state != nullptr) && mmu_state->mmu_is_present) {
      const uint32_t fsr_from_dcache = (uint32_t) (request.data >> 32);
      const uint32_t far_from_dcache = (uint32_t) request.data;
      updateFsrFar(mmu_state, thread_id, fsr_from_dcache, far_from_dcache, kIaccessFault);
    }
    StepTask resp = push_client_response(active_client, response);
    CO_AWAIT_OWNED(this, resp);
    co_return response;
  }

  if (!isValidMmuRequest(asi_7b)) {
    response.mae = 1;
    StepTask resp = push_client_response(active_client, response);
    CO_AWAIT_OWNED(this, resp);
    co_return response;
  }

  if (isCacheRelatedRequest(asi_7b)) {
    if ((mmu_state != nullptr) && mmu_state->mmu_is_present) {
      if (ASI_ICACHE_FLUSH(asi_7b)) {
        flushRlutInManager((int) mmu_state->core_id, 1);
      }
      if (ASI_DCACHE_FLUSH(asi_7b)) {
        flushRlutInManager((int) mmu_state->core_id, 0);
      }
    }
    StepTask resp = push_client_response(active_client, response);
    CO_AWAIT_OWNED(this, resp);
    co_return response;
  }

  if (asi_7b == ASI_MMU_FLUSH_PROBE) {
    if ((mmu_state != nullptr) && mmu_state->mmu_is_present) {
      if (req_type == REQUEST_TYPE_WRITE) {
        initializeTlbNew(mmu_state);
        if (global_enable_statistic_collection) {
          mmu_state->Num_Mmu_flush_requests[thread_id] += 1;
        }
      } else if (req_type == REQUEST_TYPE_READ) {
        uint32_t pte = 0;
        StepTaskT<int> probe = perform_probe(lock_mask, thread_id, request.addr, asi_7b, req_type, &pte);
        probe.set_owner(this);
        const int probe_success = co_await probe;
        if (probe_success) {
          response.data = pte;
          if ((request.addr & 0x4u) == 0u) {
            response.data <<= 32;
          }
        }
        if (global_enable_statistic_collection) {
          mmu_state->Num_Mmu_probe_requests[thread_id] += 1;
        }
      }
    }
    StepTask resp = push_client_response(active_client, response);
    CO_AWAIT_OWNED(this, resp);
    co_return response;
  }

  if (asi_7b == ASI_MMU_REGISTER) {
    if ((mmu_state != nullptr) && mmu_state->mmu_is_present) {
      if (req_type == REQUEST_TYPE_READ) {
        const uint32_t rval = read_mmu_register_legacy(mmu_state, thread_id, request.addr);
        read_data = ((uint64_t) rval) << 32;
        if (global_enable_statistic_collection) {
          mmu_state->Num_Mmu_register_reads[thread_id] += 1;
        }
      } else if (req_type == REQUEST_TYPE_WRITE) {
        write_mmu_register_legacy(mmu_state, thread_id, request.addr, request.byte_mask, request.data);
        if (global_enable_statistic_collection) {
          mmu_state->Num_Mmu_register_writes[thread_id] += 1;
        }
      }
    }
    response.data = read_data;
    StepTask resp = push_client_response(active_client, response);
    CO_AWAIT_OWNED(this, resp);
    co_return response;
  }

  uint32_t req_addr = request.addr;
  uint8_t translation_used = 0;
  if ((req_type == REQUEST_TYPE_IFETCH ||
       req_type == REQUEST_TYPE_READ ||
       req_type == REQUEST_TYPE_WRITE) &&
      (asi_7b == ASI_USER_DATA || asi_7b == ASI_SUPERVISOR_DATA ||
       asi_7b == ASI_USER_INSTRUCTION || asi_7b == ASI_SUPERVISOR_INSTRUCTION) &&
      (mmu_state != nullptr) &&
      mmu_state->mmu_is_present &&
      ((mmu_state->MmuControlRegister[thread_id] & 0x1u) != 0u)) {
    uint32_t pte = 0;
    uint8_t pte_level = 0;
    uint8_t pte_found = 0;
    uint64_t phy_addr_of_pte = 0;
    const uint32_t va_tag = get_slice32_u(request.addr, 31, 12);
    if (TLB_ENABLED) {
      pte_found = isTlbNewHit(mmu_state, thread_id, request.addr, va_tag, &pte, &pte_level);
      if (pte_found && global_enable_statistic_collection) {
        mmu_state->Num_Mmu_TLB_hits[thread_id] += 1;
      }
    }
    if (!pte_found) {
      int walk_found = 0;
      StepTaskT<int> w = walk_page_tables(lock_mask,
                                          thread_id,
                                          request.addr,
                                          &pte,
                                          &pte_level,
                                          &phy_addr_of_pte);
      w.set_owner(this);
      walk_found = co_await w;
      pte_found = (walk_found != 0);
      if (pte_found && TLB_ENABLED) {
        writeTlbNewEntry(mmu_state, thread_id, request.addr, va_tag, pte, pte_level);
      }
    }

    uint32_t fsr_to_cache = 0;
    if (!checkPageFaults(mmu_state,
                         thread_id,
                         pte_found,
                         pte,
                         pte_level,
                         asi_7b,
                         request.addr,
                         req_type,
                         &fsr_to_cache)) {
      const uint8_t nf = (uint8_t) ((mmu_state->MmuControlRegister[thread_id] >> 1) & 0x1u);
      response.mmu_fsr = fsr_to_cache;
      response.mae = (uint8_t) ((nf == 0) || (asi_7b == 0x09));
      StepTask resp = push_client_response(active_client, response);
      CO_AWAIT_OWNED(this, resp);
      co_return response;
    }

    req_addr = (uint32_t) constructPhysicalAddr(pte, pte_level, request.addr);
    response.cacheable = isCacheable(pte);
    response.acc = (uint8_t) get_slice32_u(pte, 4, 2);
    translation_used = 1;
  }

  if (command == MMU_READ_LINE) {
    const uint32_t line_addr =
        req_addr & ~((1u << LOG_BYTES_PER_CACHE_LINE) - 1u);
    const uint8_t line_req_type =
        (uint8_t) ((request.kind & 0x40u) | REQUEST_TYPE_READ);
    const uint8_t effective_cacheable =
        (uint8_t) (!translation_used || (response.cacheable != 0));

    if (effective_cacheable && (mmu_state != nullptr)) {
      StepTaskT<void> coh_task =
          register_coherence_fill(this,
                                  mmu_state,
                                  thread_id,
                                  (uint8_t) (ASI_ICACHE_VALID(asi_7b) ? 1 : 0),
                                  (req_addr >> LOG_BYTES_PER_CACHE_LINE),
                                  (request.addr >> LOG_BYTES_PER_CACHE_LINE));
      CO_AWAIT_OWNED(this, coh_task);
    }

    // Compatibility mode preserves checkpoint MMU response semantics.
    // Only async uses translated permissions/cacheability and refill metadata.
    if (!modular_async_enabled() || !translation_used) {
      response.cacheable = 1;
      response.acc = 3;
    }
    const unsigned critical_word = modular_async_enabled() ? ((request.addr >> 3) & 7u) : 0u;
    const unsigned beats = (!modular_async_enabled() || effective_cacheable) ? 8u : 1u;
    for (unsigned beat = 0; beat < beats; ++beat) {
      const unsigned word = (critical_word + beat) & 7u;
      const ModularBridgeRequest bridge_request = {
          request.source,
          thread_id,
          line_req_type,
          (uint32_t) (line_addr + word * 8u),
          request.data,
          request.byte_mask,
      };
      bridge_line_words[source]++;
      StepTaskT<ModularResponse> bridge_task = bridge_access_over_nets(bridge_request);
      bridge_task.set_owner(this);
      ModularResponse bridge_response = co_await bridge_task;
      response.mae = bridge_response.mae;
      response.data = bridge_response.data;
      response.meta = (uint64_t) (line_addr + word * 8u) |
          ((beat + 1 == beats || response.mae) ? MOD_MMU_LAST : 0) |
          ((effective_cacheable && !response.mae) ? MOD_MMU_AUTHORIZED : 0) |
          ((uint64_t) word << 34);
      StepTask resp = push_client_response(active_client, response);
      CO_AWAIT_OWNED(this, resp);
      if (response.mae != 0) {
        co_return response;
      }
    }
    if (translation_used && mmu_enabled(mmu_state, thread_id) &&
        global_enable_statistic_collection && (mmu_state != nullptr)) {
      mmu_state->Num_Mmu_translated_accesses[thread_id] += 1;
    }
    co_return response;
  }

  if ((command == MMU_READ_DWORD) ||
      (command == MMU_WRITE_DWORD) ||
      (command == MMU_WRITE_DWORD_NO_RESPONSE)) {
    const ModularBridgeRequest bridge_request = {
        request.source,
        thread_id,
        request.kind,
        req_addr,
        request.data,
        request.byte_mask,
    };
    StepTaskT<ModularResponse> bridge_task = bridge_access_over_nets(bridge_request);
    bridge_task.set_owner(this);
    ModularResponse bridge_response = co_await bridge_task;
    response.mae = bridge_response.mae;
    if (command == MMU_READ_DWORD) {
      response.data = bridge_response.data;
    }
    // Compatibility mode preserves checkpoint MMU response semantics.
    // Only async uses translated permissions/cacheability and refill metadata.
    if (!modular_async_enabled() || !translation_used) {
      response.cacheable = 1;
      response.acc = 3;
    }
    response.meta = (uint64_t) req_addr | MOD_MMU_LAST |
        ((response.cacheable && !response.mae && !lock_mask) ? MOD_MMU_AUTHORIZED : 0);
    if (translation_used && mmu_enabled(mmu_state, thread_id) &&
        global_enable_statistic_collection && (mmu_state != nullptr)) {
      mmu_state->Num_Mmu_translated_accesses[thread_id] += 1;
    }
    StepTask resp = push_client_response(active_client, response);
    CO_AWAIT_OWNED(this, resp);
    co_return response;
  }

  response.mae = 1;
  StepTask resp = push_client_response(active_client, response);
  CO_AWAIT_OWNED(this, resp);
  co_return response;
}
