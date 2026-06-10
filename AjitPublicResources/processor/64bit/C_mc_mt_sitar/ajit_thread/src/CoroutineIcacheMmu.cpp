#include "../include/CoroutineIcacheMmu.h"
#include "../../shim/ajit_memory_shim.h"
#include "../../shim/ajit_thread_bridge.h"
#include "../../shim/deep_fetch_step.h"
#include <cstdio>
#include <pthread.h>
#include <errno.h>
#include <cstdlib>

extern "C" {
#include "RequestTypeValues.h"
#include "ASI_values.h"
#include "TlbNew.h"
extern uint64_t ajit_sitar_sim_time;
extern int global_enable_statistic_collection;
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
}

namespace {

inline int cache_line_offset(uint32_t addr) {
  return (int) ((addr >> 3) & 0x7u);
}

inline int thread_slot_id(MmuState* ms, int thread_id)
{
  const int threads_per_core = ajit_thread_bridge_get_threads_per_core();
  const int tpc = (threads_per_core > 0) ? threads_per_core : 1;
  const int core_id = (ms != nullptr) ? (int) ms->core_id : 0;
  return (core_id * tpc) + thread_id;
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

inline uint32_t get_slice32_u(uint32_t x, uint8_t hi, uint8_t lo)
{
  const uint32_t width = (uint32_t) (hi - lo + 1u);
  const uint32_t mask = (width >= 32u) ? 0xffffffffu : ((1u << width) - 1u);
  return (x >> lo) & mask;
}

inline uint64_t get_phy_addr_from_ptd(uint32_t ptd, uint32_t index)
{
  // Legacy Mmu.c getPhyAddrFromPTD()
  uint64_t phy_addr = (uint64_t) (ptd & ~0x3u);
  phy_addr <<= 4;
  phy_addr |= ((uint64_t) index << 2);
  return phy_addr;
}

StepTaskT<int> read_pt_entry_coroutine(CoroutineOwner* owner,
                                       MmuState* ms,
                                       int thread_id,
                                       uint8_t lock_mask,
                                       uint64_t physical_addr,
                                       uint32_t* pte_out)
{
  uint64_t data64 = 0;
  StepTaskT<int> req = sysMemBusRequest(owner,
                                        (int) ms->core_id,
                                        thread_id,
                                        (uint8_t) (REQUEST_TYPE_READ | lock_mask),
                                        0,
                                        (uint32_t) physical_addr,
                                        0,
                                        &data64);
  int ok = 0;
  CO_AWAIT_OWNED_VAL(ok, owner, req);
  if (!ok) {
    co_return 0;
  }
  if ((physical_addr & 0x4ull) == 0ull) {
    *pte_out = (uint32_t) (data64 >> 32);
  } else {
    *pte_out = (uint32_t) data64;
  }
  co_return 1;
}

StepTaskT<int> walk_page_tables_coroutine(CoroutineOwner* owner,
                                          MmuState* ms,
                                          int thread_id,
                                          uint8_t lock_mask,
                                          uint32_t virt_addr,
                                          uint32_t* pte,
                                          uint8_t* pte_level)
{
  const int tid = (thread_id & MMU_THREAD_MASK);
  const uint32_t context_table_index = ms->MmuContextRegister[tid];
  const uint32_t l1_index = get_slice32_u(virt_addr, 31, 24);
  const uint32_t l2_index = get_slice32_u(virt_addr, 23, 18);
  const uint32_t l3_index = get_slice32_u(virt_addr, 17, 12);

  uint64_t phy_addr = get_phy_addr_from_ptd(ms->MmuContextTablePointerRegister[tid], context_table_index);
  uint32_t e = 0;
  int ok = 0;
  StepTaskT<int> r0 = read_pt_entry_coroutine(owner, ms, thread_id, lock_mask, phy_addr, &e);
  CO_AWAIT_OWNED_VAL(ok, owner, r0);
  if (!ok) co_return 0;
  *pte_level = 0;

  if ((e & 0x3u) == 0x1u) {
    phy_addr = get_phy_addr_from_ptd(e, l1_index);
    StepTaskT<int> r1 = read_pt_entry_coroutine(owner, ms, thread_id, lock_mask, phy_addr, &e);
    CO_AWAIT_OWNED_VAL(ok, owner, r1);
    if (!ok) co_return 0;
    *pte_level = 1;

    if ((e & 0x3u) == 0x1u) {
      phy_addr = get_phy_addr_from_ptd(e, l2_index);
      StepTaskT<int> r2 = read_pt_entry_coroutine(owner, ms, thread_id, lock_mask, phy_addr, &e);
      CO_AWAIT_OWNED_VAL(ok, owner, r2);
      if (!ok) co_return 0;
      *pte_level = 2;

      if ((e & 0x3u) == 0x1u) {
        phy_addr = get_phy_addr_from_ptd(e, l3_index);
        StepTaskT<int> r3 = read_pt_entry_coroutine(owner, ms, thread_id, lock_mask, phy_addr, &e);
        CO_AWAIT_OWNED_VAL(ok, owner, r3);
        if (!ok) co_return 0;
        *pte_level = 3;
      }
    }
  }

  *pte = e;
  co_return ((e & 0x3u) == 0x2u);
}

// Legacy MMU register access behavior (Mmu.c: readMmuRegister/writeMmuRegister).
inline uint32_t mmu_reg_index(uint32_t addr)
{
  return (addr >> 8) & 0xFFFFFFu;
}

inline int mmu_tid_index(int thread_id)
{
  return (thread_id & MMU_THREAD_MASK);
}

inline uint32_t read_mmu_register_legacy(MmuState* ms, int thread_id, uint32_t addr)
{
  if (ms == nullptr) {
    return 0;
  }
  const int tid = mmu_tid_index(thread_id);
  const uint32_t reg = mmu_reg_index(addr);
  switch (reg) {
    case 0:
      return ms->MmuControlRegister[tid];
    case 1:
      return ms->MmuContextTablePointerRegister[tid];
    case 2:
      return ms->MmuContextRegister[tid];
    case 3: {
      const uint32_t v = ms->MmuFaultStatusRegister[tid];
      // Reading FSR clears it in legacy.
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

enum LockCounterKind {
  LOCK_COUNTER_ICACHE = 0,
  LOCK_COUNTER_DCACHE = 1,
  LOCK_COUNTER_MMU = 2
};

enum MmuSourceKind {
  MMU_SOURCE_IFETCH = 0,
  MMU_SOURCE_DCACHE = 1
};

static volatile uint64_t g_lock_busy_counts[3][4] = {};
static volatile uint64_t g_dcache_owner_busy_counts[4] = {};
static volatile uint64_t g_mmu_busy_by_source[2][4] = {};
static volatile uint64_t g_mmu_req_by_source[2][4] = {};
static volatile uint64_t g_lock_busy_counts_by_phase[3][2][4] = {};
static volatile uint64_t g_dcache_owner_busy_counts_by_phase[2][4] = {};
static volatile uint64_t g_mmu_busy_by_source_phase[2][2][4] = {};

inline int sim_phase_index()
{
  return (int) (ajit_sitar_sim_time & 0x1ull);
}

inline void bump_lock_busy(LockCounterKind kind, int core_id)
{
  if ((core_id >= 0) && (core_id < 4)) {
    __sync_fetch_and_add(&g_lock_busy_counts[(int) kind][core_id], 1ULL);
    __sync_fetch_and_add(&g_lock_busy_counts_by_phase[(int) kind][sim_phase_index()][core_id], 1ULL);
  }
}

inline void bump_dcache_owner_busy(int core_id)
{
  if ((core_id >= 0) && (core_id < 4)) {
    __sync_fetch_and_add(&g_dcache_owner_busy_counts[core_id], 1ULL);
    __sync_fetch_and_add(&g_dcache_owner_busy_counts_by_phase[sim_phase_index()][core_id], 1ULL);
  }
}

inline bool phase_lock_mode_enabled()
{
  static int initialized = 0;
  static int enabled = 0;
  if (!initialized) {
    const char* v = std::getenv("AJIT_PHASE_LOCKING");
    enabled = (v && (v[0] == '1'));
    initialized = 1;
  }
  return enabled != 0;
}

StepTaskT<void> unlock_or_yield_phase1(CoroutineOwner* owner, pthread_mutex_t* m)
{
  if (m == nullptr) {
    co_return;
  }
  while (phase_lock_mode_enabled() && ((ajit_sitar_sim_time & 0x1ull) == 0ull)) {
    co_await yield_point(owner);
  }
  pthread_mutex_unlock(m);
  co_return;
}

// Wrapper-style mutex admission: one trylock attempt per simulation step.
StepTaskT<int> mutex_trylock_or_yield(CoroutineOwner* owner,
                                      pthread_mutex_t* m,
                                      LockCounterKind kind,
                                      int core_id)
{
  while (1) {
    if (phase_lock_mode_enabled() && ((ajit_sitar_sim_time & 0x1ull) != 0ull)) {
      co_await yield_point(owner);
      continue;
    }
    int rc = pthread_mutex_trylock(m);
    if (rc == 0) {
      co_return 1;
    }
    if (rc != EBUSY) {
      co_return 0;
    }
    bump_lock_busy(kind, core_id);
    co_await yield_point(owner);
  }
}

StepTaskT<int> lock_mmu(CoroutineOwner* owner, MmuState* ms, MmuSourceKind source)
{
  if (ms == nullptr) {
    co_return 1;
  }
  while (1) {
    if (phase_lock_mode_enabled() && ((ajit_sitar_sim_time & 0x1ull) != 0ull)) {
      co_await yield_point(owner);
      continue;
    }
    int rc = pthread_mutex_trylock(&ms->mmu_mutex);
    if (rc == 0) {
      co_return 1;
    }
    if (rc != EBUSY) {
      co_return 0;
    }
    const int core_id = (int) ms->core_id;
    bump_lock_busy(LOCK_COUNTER_MMU, core_id);
    if ((core_id >= 0) && (core_id < 4)) {
      __sync_fetch_and_add(&g_mmu_busy_by_source[(int) source][core_id], 1ULL);
      __sync_fetch_and_add(&g_mmu_busy_by_source_phase[(int) source][sim_phase_index()][core_id], 1ULL);
    }
    co_await yield_point(owner);
  }
}

StepTaskT<int> lock_icache(CoroutineOwner* owner,
                           WriteThroughAllocateCache* icache,
                           int core_id)
{
  if (icache == nullptr) {
    co_return 1;
  }
  StepTaskT<int> lk = mutex_trylock_or_yield(owner,
                                             &icache->cache_mutex,
                                             LOCK_COUNTER_ICACHE,
                                             core_id);
  int locked = 0;
  CO_AWAIT_OWNED_VAL(locked, owner, lk);
  co_return locked;
}

StepTaskT<int> lock_dcache(CoroutineOwner* owner, WriteThroughAllocateCache* dcache, int core_id)
{
  if (dcache == nullptr) {
    co_return 1;
  }

  // Match legacy admission policy for lock_flag ownership.
  while (1) {
    StepTaskT<int> lk = mutex_trylock_or_yield(owner,
                                               &dcache->cache_mutex,
                                               LOCK_COUNTER_DCACHE,
                                               core_id);
    int locked = 0;
    CO_AWAIT_OWNED_VAL(locked, owner, lk);
    if (!locked) {
      co_return 0;
    }
    if (!dcache->lock_flag || (dcache->lock_core_id == core_id)) {
      co_return 1;
    }
    bump_dcache_owner_busy(core_id);
    StepTaskT<void> unl = unlock_or_yield_phase1(owner, &dcache->cache_mutex);
    CO_AWAIT_OWNED(owner, unl);
    co_await yield_point(owner);
  }
}

} // namespace

StepTaskT<void> Mmu(CoroutineOwner* owner,
                    MmuState* ms,
                    int thread_id,
                    uint8_t mmu_command,
                    uint8_t request_type,
                    uint8_t asi,
                    uint32_t addr,
                    uint8_t byte_mask,
                    uint64_t write_data,
                    uint8_t* mae,
                    uint8_t* cacheable,
                    uint8_t* acc,
                    uint64_t* read_data,
                    uint32_t* mmu_fsr,
                    uint32_t* synonym_invalidate_word)
{
  if (mae) *mae = 0;
  if (cacheable) *cacheable = 0;
  if (acc) *acc = 0;
  if (mmu_fsr) *mmu_fsr = 0;
  if (synonym_invalidate_word) *synonym_invalidate_word = 0;

  // Contention model for shared per-core MMU.
  int mmu_lock_acquired = 0;
  const MmuSourceKind mmu_source =
      (request_type == REQUEST_TYPE_IFETCH) ? MMU_SOURCE_IFETCH : MMU_SOURCE_DCACHE;
  if ((ms != nullptr) && ((int) ms->core_id >= 0) && ((int) ms->core_id < 4)) {
    __sync_fetch_and_add(&g_mmu_req_by_source[(int) mmu_source][(int) ms->core_id], 1ULL);
  }
  StepTaskT<int> mmu_lk = lock_mmu(owner, ms, mmu_source);
  CO_AWAIT_OWNED_VAL(mmu_lock_acquired, owner, mmu_lk);
  if (!mmu_lock_acquired) {
    if (mae) *mae = 1;
    co_return;
  }

  // Legacy request decode uses lower 6 bits for type and lower 7 bits for ASI.
  const uint8_t req_type = (uint8_t) (request_type & 0x3fu);
  const uint8_t lock_mask = (uint8_t) (request_type & 0x40u);
  const uint8_t asi_7b = (uint8_t) (asi & 0x7fu);

  // Legacy validation path.
  if (!isValidMmuRequest(asi_7b)) {
    if (mae) *mae = 1;
    if (cacheable) *cacheable = 0;
    if (acc) *acc = 0;
    if (read_data) *read_data = 0;
    if (mmu_fsr) *mmu_fsr = 0;
    if (mmu_lock_acquired && (ms != nullptr)) {
      StepTaskT<void> unl = unlock_or_yield_phase1(owner, &ms->mmu_mutex);
      CO_AWAIT_OWNED(owner, unl);
    }
    co_return;
  }

  // Legacy CASE 1: cache-related ASIs are handled in MMU (RLUT flush) and
  // must return a dummy response without memory bus traffic.
  if (isCacheRelatedRequest(asi_7b)) {
    if ((ms != nullptr) && ms->mmu_is_present) {
      if (ASI_ICACHE_FLUSH(asi_7b)) {
        flushRlutInManager((int) ms->core_id, 1);
      }
      if (ASI_DCACHE_FLUSH(asi_7b)) {
        flushRlutInManager((int) ms->core_id, 0);
      }
    }
    if (mae) *mae = 0;
    if (cacheable) *cacheable = 0;
    if (acc) *acc = 0;
    if (read_data) *read_data = 0;
    if (mmu_fsr) *mmu_fsr = 0;
    if (mmu_lock_acquired && (ms != nullptr)) {
      StepTaskT<void> unl = unlock_or_yield_phase1(owner, &ms->mmu_mutex);
      CO_AWAIT_OWNED(owner, unl);
    }
    co_return;
  }

  // Legacy CASE 4: ASI_MMU_REGISTER accesses are handled entirely inside MMU
  // and must not touch system memory.
  if (asi_7b == ASI_MMU_REGISTER) {
    if ((ms != nullptr) && ms->mmu_is_present) {
      if (req_type == REQUEST_TYPE_READ) {
        const uint32_t rval = read_mmu_register_legacy(ms, thread_id, addr);
        if (read_data) {
          // Legacy behavior: register value appears on bits [63:32].
          *read_data = ((uint64_t) rval) << 32;
        }
        if (global_enable_statistic_collection) {
          const int tid = mmu_tid_index(thread_id);
          ms->Num_Mmu_register_reads[tid] += 1;
        }
      } else if (req_type == REQUEST_TYPE_WRITE) {
        write_mmu_register_legacy(ms, thread_id, addr, byte_mask, write_data);
        if (global_enable_statistic_collection) {
          const int tid = mmu_tid_index(thread_id);
          ms->Num_Mmu_register_writes[tid] += 1;
        }
      }
    }
    if (mae) *mae = 0;
    if (cacheable) *cacheable = 0;
    if (acc) *acc = 0;
    if (mmu_fsr) *mmu_fsr = 0;
    if (mmu_lock_acquired) {
      StepTaskT<void> unl = unlock_or_yield_phase1(owner, &ms->mmu_mutex);
      CO_AWAIT_OWNED(owner, unl);
    }
    co_return;
  }

  // Legacy CASE 3: ASI_MMU_FLUSH_PROBE (partial parity).
  // We preserve flush behavior and safe probe read response shape.
  if (asi_7b == ASI_MMU_FLUSH_PROBE) {
    if ((ms != nullptr) && ms->mmu_is_present) {
      const int tid = mmu_tid_index(thread_id);
      if (req_type == REQUEST_TYPE_WRITE) {
        initializeTlbNew(ms);
        if (global_enable_statistic_collection) {
          ms->Num_Mmu_flush_requests[tid] += 1;
        }
      } else if (req_type == REQUEST_TYPE_READ) {
        if (global_enable_statistic_collection) {
          ms->Num_Mmu_probe_requests[tid] += 1;
        }
      }
    }
    if (mae) *mae = 0;
    if (cacheable) *cacheable = 0;
    if (acc) *acc = 0;
    if (mmu_fsr) *mmu_fsr = 0;
    if (read_data) *read_data = 0;
    if (mmu_lock_acquired && (ms != nullptr)) {
      StepTaskT<void> unl = unlock_or_yield_phase1(owner, &ms->mmu_mutex);
      CO_AWAIT_OWNED(owner, unl);
    }
    co_return;
  }

  // Coroutine path for memory requests that must use deep-yield bus access.
  // MMU is shared per core between I$/D$ requesters.
  uint32_t req_addr = addr;
  uint8_t translation_used = 0;
  if ((req_type == REQUEST_TYPE_IFETCH ||
       req_type == REQUEST_TYPE_READ ||
       req_type == REQUEST_TYPE_WRITE) &&
      (asi_7b == ASI_USER_DATA || asi_7b == ASI_SUPERVISOR_DATA ||
       asi_7b == ASI_USER_INSTRUCTION || asi_7b == ASI_SUPERVISOR_INSTRUCTION) &&
      (ms != nullptr) &&
      ms->mmu_is_present &&
      (((ms->MmuControlRegister[mmu_tid_index(thread_id)] & 0x1u) != 0u)))
  {
    uint32_t pte = 0;
    uint8_t pte_level = 0;
    uint8_t pte_found = 0;
    uint32_t va_tag = get_slice32_u(addr, 31, 12);
    if (TLB_ENABLED) {
      pte_found = isTlbNewHit(ms, thread_id, addr, va_tag, &pte, &pte_level);
      if (pte_found && global_enable_statistic_collection) {
        ms->Num_Mmu_TLB_hits[mmu_tid_index(thread_id)] += 1;
      }
    }
    if (!pte_found) {
      int walk_found = 0;
      StepTaskT<int> w = walk_page_tables_coroutine(owner, ms, thread_id, lock_mask, addr, &pte, &pte_level);
      CO_AWAIT_OWNED_VAL(walk_found, owner, w);
      if (!walk_found) {
        pte_found = 0;
      } else {
        pte_found = 1;
        if (TLB_ENABLED) {
          writeTlbNewEntry(ms, thread_id, addr, va_tag, pte, pte_level);
        }
      }
    }

    uint32_t fsr_to_cache = 0;
    if (!checkPageFaults(ms, thread_id, pte_found, pte, pte_level, asi_7b, addr, req_type, &fsr_to_cache)) {
      const uint8_t NF = (uint8_t) ((ms->MmuControlRegister[mmu_tid_index(thread_id)] >> 1) & 0x1u);
      if (mmu_fsr) *mmu_fsr = fsr_to_cache;
      if (cacheable) *cacheable = 0;
      if (acc) *acc = 0;
      if (read_data) *read_data = 0;
      if (mae) *mae = (uint8_t) ((NF == 0) || (asi_7b == 0x09));
      if (mmu_lock_acquired) {
        StepTaskT<void> unl = unlock_or_yield_phase1(owner, &ms->mmu_mutex);
        CO_AWAIT_OWNED(owner, unl);
      }
      co_return;
    }

    req_addr = (uint32_t) constructPhysicalAddr(pte, pte_level, addr);
    if (cacheable) *cacheable = isCacheable(pte);
    if (acc) *acc = (uint8_t) get_slice32_u(pte, 4, 2);
    translation_used = 1;
  }

  if ((mmu_command == MMU_READ_LINE) ||
      (mmu_command == MMU_READ_DWORD) ||
      (mmu_command == MMU_WRITE_DWORD) ||
      (mmu_command == MMU_WRITE_DWORD_NO_RESPONSE) ||
      (mmu_command == MMU_WRITE_FSR))
  {
    if (mmu_command == MMU_READ_LINE) {
      uint32_t line_addr = (req_addr & LINE_ADDR_MASK);
      const uint32_t physical_line_addr = (req_addr >> LOG_BYTES_PER_CACHE_LINE);
      const uint32_t virtual_line_addr = (addr >> LOG_BYTES_PER_CACHE_LINE);
      const uint8_t line_req_type = (uint8_t) ((request_type & 0x40u) | REQUEST_TYPE_READ);
      const uint8_t effective_cacheable =
          (uint8_t) (!translation_used || (cacheable && (*cacheable != 0)));
      if (read_data) {
        for (int i = 0; i < 8; ++i) {
          read_data[i] = 0;
        }
      }
      if (effective_cacheable && (ms != nullptr)) {
        StepTaskT<void> coh_task = register_coherence_fill(owner,
                                                           ms,
                                                           thread_id,
                                                           (uint8_t) (ASI_ICACHE_VALID(asi_7b) ? 1 : 0),
                                                           physical_line_addr,
                                                           virtual_line_addr);
        CO_AWAIT_OWNED(owner, coh_task);
      }
      for (int i = 0; i < 8; ++i) {
        uint64_t rd = 0;
        StepTaskT<int> req = sysMemBusRequest(owner,
                                              (int) (ms ? ms->core_id : 0),
                                              thread_id,
                                              line_req_type,
                                              byte_mask,
                                              line_addr + ((uint32_t) i * 8u),
                                              write_data,
                                              &rd);
        int ok = 0;
        CO_AWAIT_OWNED_VAL(ok, owner, req);
        if (!ok) {
          if (mae) *mae = 1;
          if (mmu_lock_acquired) {
            StepTaskT<void> unl = unlock_or_yield_phase1(owner, &ms->mmu_mutex);
            CO_AWAIT_OWNED(owner, unl);
          }
          co_return;
        }
        if (read_data) {
          read_data[i] = rd;
        }
      }
    } else {
      uint64_t rd = 0;
      StepTaskT<int> req = sysMemBusRequest(owner,
                                            (int) (ms ? ms->core_id : 0),
                                            thread_id,
                                            request_type,
                                            byte_mask,
                                            req_addr,
                                            write_data,
                                            &rd);
      int ok = 0;
      CO_AWAIT_OWNED_VAL(ok, owner, req);
      if (!ok) {
        if (mae) *mae = 1;
        if (mmu_lock_acquired) {
          StepTaskT<void> unl = unlock_or_yield_phase1(owner, &ms->mmu_mutex);
          CO_AWAIT_OWNED(owner, unl);
        }
        co_return;
      }
      if (read_data && (mmu_command == MMU_READ_DWORD)) {
        *read_data = rd;
      }
    }

    if (cacheable) *cacheable = 1;
    if (acc) *acc = 3;
    if (mae) *mae = 0;
    if (mmu_lock_acquired) {
      StepTaskT<void> unl = unlock_or_yield_phase1(owner, &ms->mmu_mutex);
      CO_AWAIT_OWNED(owner, unl);
    }
    co_return;
  }

  // Keep StepTask path non-blocking: do not fall back to legacy blocking MMU.
  if (mae) *mae = 1;
  if (cacheable) *cacheable = 0;
  if (acc) *acc = 0;
  if (read_data) {
    // For MMU_READ_LINE callers, clear the whole cache-line payload.
    if (mmu_command == MMU_READ_LINE) {
      for (int i = 0; i < 8; ++i) read_data[i] = 0;
    } else {
      *read_data = 0;
    }
  }
#ifdef DEBUG
  std::fprintf(stderr,
               "Mmu-StepTask: unsupported mmu_command=0x%x req=0x%x core=%u tid=%d addr=0x%x\n",
               mmu_command, request_type, (ms ? ms->core_id : 0), thread_id, addr);
#endif
  if (mmu_lock_acquired) {
    StepTaskT<void> unl = unlock_or_yield_phase1(owner, &ms->mmu_mutex);
    CO_AWAIT_OWNED(owner, unl);
  }
  co_return;
}

StepTaskT<void> cpuIcacheAccess(CoroutineOwner* owner,
                                int core_id,
                                int cpu_id,
                                uint8_t context,
                                MmuState* ms,
                                WriteThroughAllocateCache* icache,
                                uint8_t asi,
                                uint32_t addr,
                                uint8_t request_type,
                                uint8_t byte_mask,
                                uint8_t* mae,
                                uint64_t* instr_pair,
                                uint32_t* mmu_fsr)
{
  (void) core_id;

  if (mae) *mae = 0;
  if (instr_pair) *instr_pair = 0;
  if (mmu_fsr) *mmu_fsr = 0;

  // Contention model for shared per-core I-cache.
  int lock_acquired = 0;
  StepTaskT<int> cache_lk = lock_icache(owner, icache, core_id);
  CO_AWAIT_OWNED_VAL(lock_acquired, owner, cache_lk);
  if (!lock_acquired) {
    if (mae) *mae = 1;
    co_return;
  }

  // Legacy behavior: service coherency invalidates before request handling.
  if ((ms != nullptr) && (icache != nullptr)) {
    while (1) {
      uint32_t invalidate_word = probeCoherencyInvalidateRequest(ms->core_id, 1);
      if (invalidate_word == 0) {
        break;
      }
      uint32_t invalidate_va =
          (invalidate_word & 0x7fffffffu) << LOG_BYTES_PER_CACHE_LINE;
      invalidateCacheLine(icache, invalidate_va);
    }
  }

  const uint8_t asi_7b = (uint8_t) (asi & 0x7f);
  const uint8_t req = (uint8_t) (request_type & 0x3f);
  uint8_t is_nop = 0, is_flush = 0, is_ifetch = 0;
  decodeIcacheRequest(asi_7b, req, &is_nop, &is_flush, &is_ifetch);
  if (icache != nullptr) {
    icache->number_of_accesses++;
  }

  if (is_nop) {
    if (mae) *mae = 0;
    if (instr_pair) *instr_pair = 0;
    if (lock_acquired) {
      StepTaskT<void> unl = unlock_or_yield_phase1(owner, &icache->cache_mutex);
      CO_AWAIT_OWNED(owner, unl);
    }
    co_return;
  }

  // Coroutine path for ifetch.
  if (is_ifetch) {
    if ((icache != nullptr) && !icache->multi_context) {
      context = 0;
    }

    uint8_t acc = 0;
    uint8_t is_hit = 0;
    if (icache != nullptr) {
      uint8_t is_raw_hit = 0;
      lookupCache(icache, context, addr, asi_7b, &is_raw_hit, &acc);
      const int access_ok = accessPermissionsOk(REQUEST_TYPE_IFETCH, asi_7b, acc);
      is_hit = (uint8_t) (is_raw_hit && access_ok);
    }

    if (is_hit) {
      if (icache != nullptr) {
        icache->number_of_hits++;
      }
      if (instr_pair) {
        *instr_pair = getDwordFromCache(icache, context, addr);
      }
      if (mae) {
        *mae = (uint8_t) ((1u << 7) | ((acc & 0x7u) << 4));
      }
    } else {
      if (icache != nullptr) {
        icache->number_of_misses++;
      }
      uint8_t cacheable = 0, lmae = 0;
      uint64_t line_data[8] = {};
      uint32_t synonym = 0;
      StepTaskT<void> mmu_task = Mmu(owner, ms, cpu_id,
                                     MMU_READ_LINE,
                                     REQUEST_TYPE_IFETCH,
                                     asi_7b,
                                     addr,
                                     byte_mask,
                                     0,
                                     &lmae,
                                     &cacheable,
                                     &acc,
                                     line_data,
                                     mmu_fsr,
                                     &synonym);
      CO_AWAIT_OWNED(owner, mmu_task);

      if (synonym != 0 && icache != nullptr) {
        uint32_t invalidate_va =
            (synonym & 0x7fffffffu) << LOG_BYTES_PER_CACHE_LINE;
        invalidateCacheLine(icache, invalidate_va);
      }

      if (instr_pair) {
        if (cacheable && icache != nullptr) {
          uint32_t line_addr = (addr & LINE_ADDR_MASK);
          updateCacheLine(icache, acc, context, line_addr, line_data);
          *instr_pair = getDwordFromCache(icache, context, addr);
        } else {
          *instr_pair = line_data[cacheLineOffset(addr)];
        }
      }
      if (mae) {
        *mae = (uint8_t) (((cacheable & 0x1u) << 7) |
                          ((acc & 0x7u) << 4) |
                          (lmae & 0x3u));
      }
    }

    if (lock_acquired) {
      StepTaskT<void> unl = unlock_or_yield_phase1(owner, &icache->cache_mutex);
      CO_AWAIT_OWNED(owner, unl);
    }
    co_return;
  }

  if (is_flush) {
    if (icache != nullptr) {
      icache->number_of_flushes++;
      flushCache(icache);
    }
    if (mae) *mae = 0;
    if (instr_pair) *instr_pair = 0;
    if (lock_acquired) {
      StepTaskT<void> unl = unlock_or_yield_phase1(owner, &icache->cache_mutex);
      CO_AWAIT_OWNED(owner, unl);
    }
    co_return;
  }

  // Keep StepTask path non-blocking: do not fall back to legacy blocking I$ path.
  if (mae) *mae = 1;
  if (instr_pair) *instr_pair = 0;
#ifdef DEBUG
  std::fprintf(stderr,
               "cpuIcacheAccess-StepTask: unsupported request_type=0x%x core=%d cpu=%d addr=0x%x\n",
               request_type, core_id, cpu_id, addr);
#endif
  if (lock_acquired) {
    StepTaskT<void> unl = unlock_or_yield_phase1(owner, &icache->cache_mutex);
    CO_AWAIT_OWNED(owner, unl);
  }
  co_return;
}

StepTaskT<void> cpuDcacheAccess(CoroutineOwner* owner,
                                int core_id,
                                int cpu_id,
                                uint8_t context,
                                MmuState* ms,
                                WriteThroughAllocateCache* dcache,
                                uint8_t asi,
                                uint32_t addr,
                                uint8_t request_type,
                                uint8_t byte_mask,
                                uint64_t write_data,
                                uint8_t* mae,
                                uint64_t* read_data)
{
  if (mae) *mae = 0;
  if (read_data) *read_data = 0;

  const uint8_t lock_mask = (uint8_t) (request_type & 0x40);
  int lock_acquired = 0;
  if (dcache != nullptr) {
    StepTaskT<int> dcache_lk = lock_dcache(owner, dcache, core_id);
    CO_AWAIT_OWNED_VAL(lock_acquired, owner, dcache_lk);
    if (!lock_acquired) {
      if (mae) *mae = 1;
      co_return;
    }
    // Preserve legacy lock-mask behavior.
    if (lock_mask != 0) {
      dcache->number_of_locked_accesses++;
      dcache->lock_flag = 1;
      dcache->lock_core_id = (uint8_t) core_id;
    } else {
      dcache->lock_flag = 0;
    }
  }

  // Legacy behavior: service coherency invalidates before request handling.
  if ((ms != nullptr) && (dcache != nullptr)) {
    while (1) {
      uint32_t invalidate_word = probeCoherencyInvalidateRequest(ms->core_id, 0);
      if (invalidate_word == 0) {
        break;
      }
      uint32_t invalidate_va = (invalidate_word & 0x7fffffffu);
      invalidateCacheLine(dcache, invalidate_va);
    }
  }

  uint8_t lmae = 0, cacheable = 0, acc = 0;
  uint32_t mmu_fsr = 0, synonym = 0;
  uint8_t req = (request_type & 0x3f);
  const uint8_t asi_7b = (uint8_t) (asi & 0x7f);

  // Match legacy behavior: ignore context on single-context D-cache.
  if ((dcache != nullptr) && !dcache->multi_context) {
    context = 0;
  }

  // Match legacy mapping for debugger cache requests.
  if (req == REQUEST_TYPE_CCU_CACHE_READ) {
    req = REQUEST_TYPE_READ;
  } else if (req == REQUEST_TYPE_CCU_CACHE_WRITE) {
    req = REQUEST_TYPE_WRITE;
  }

  if (dcache != nullptr) {
    dcache->number_of_accesses++;
  }

  // D-cache flush ASI path: mirror legacy decodeDcacheRequest flush behavior.
  if ((asi_7b >= 0x10u) && (asi_7b <= 0x14u)) {
    if (dcache != nullptr) {
      dcache->number_of_flushes++;
      flushCache(dcache);
    }
    if (mae) *mae = 0;
    if (read_data) *read_data = 0;
    if (lock_acquired) {
      StepTaskT<void> unl = unlock_or_yield_phase1(owner, &dcache->cache_mutex);
      CO_AWAIT_OWNED(owner, unl);
    }
    co_return;
  }

  uint8_t is_nop = 0, is_stbar = 0, is_flush = 0, is_read = 0;
  uint8_t is_cached_mem_access = 0, is_mmu_access = 0, is_mmu_bypass = 0;
  decodeDcacheRequest(asi_7b,
                      req,
                      &is_nop,
                      &is_stbar,
                      &is_flush,
                      &is_read,
                      &is_cached_mem_access,
                      &is_mmu_access,
                      &is_mmu_bypass);

  if (is_nop) {
    if (mae) *mae = 0;
    if (read_data) *read_data = 0;
    if (lock_acquired) {
      StepTaskT<void> unl = unlock_or_yield_phase1(owner, &dcache->cache_mutex);
      CO_AWAIT_OWNED(owner, unl);
    }
    co_return;
  }

  // Legacy D-cache behavior: MMU-access ASIs can induce a cache flush
  // (except context-pointer write in multi-context mode).
  if (is_mmu_access && (dcache != nullptr)) {
    dcache->number_of_flushes++;
    if (!dcache->multi_context || !isMmuContextPointerWrite(asi_7b, addr)) {
      flushCache(dcache);
    }
  }

  const uint8_t is_supported_req =
      (req == REQUEST_TYPE_READ) ||
      (req == REQUEST_TYPE_WRITE) ||
      (req == REQUEST_TYPE_WRFSRFAR) ||
      (req == REQUEST_TYPE_STBAR);
  if (!is_supported_req) {
    if (mae) *mae = 1;
    if (read_data) *read_data = 0;
#ifdef DEBUG
    std::fprintf(stderr,
                 "cpuDcacheAccess-StepTask: unsupported request_type=0x%x core=%d cpu=%d addr=0x%x\n",
                 request_type, core_id, cpu_id, addr);
#endif
    if (lock_acquired) {
      StepTaskT<void> unl = unlock_or_yield_phase1(owner, &dcache->cache_mutex);
      CO_AWAIT_OWNED(owner, unl);
    }
    co_return;
  }

  uint8_t is_hit = 0;
  if ((dcache != nullptr) && is_cached_mem_access) {
    uint8_t is_raw_hit = 0;
    lookupCache(dcache, context, addr, asi_7b, &is_raw_hit, &acc);
    const int access_ok = accessPermissionsOk(req, asi_7b, acc);
    is_hit = (uint8_t) (is_raw_hit && access_ok);

    if (is_hit && is_read) {
      dcache->number_of_hits++;
      dcache->number_of_read_hits++;
      if (read_data) {
        *read_data = getDwordFromCache(dcache, context, addr);
      }
    }
    if (is_hit && !is_read) {
      dcache->number_of_hits++;
      dcache->number_of_write_hits++;
      // Keep cache line coherent with store while write-through goes to MMU.
      writeIntoLine(dcache, write_data, context, addr, byte_mask);
    }
    if (!is_hit && is_read) {
      dcache->number_of_misses++;
      dcache->number_of_read_misses++;
    }
    if (!is_hit && !is_read) {
      dcache->number_of_misses++;
      dcache->number_of_write_misses++;
    }
  }
  else if (!is_mmu_access && (dcache != nullptr)) {
    dcache->number_of_bypasses++;
  }

  // Store barrier has no direct data payload in this path.
  if (req == REQUEST_TYPE_STBAR) {
    lmae = 0;
    if (mae) *mae = lmae;
    if (lock_acquired) {
      StepTaskT<void> unl = unlock_or_yield_phase1(owner, &dcache->cache_mutex);
      CO_AWAIT_OWNED(owner, unl);
    }
    co_return;
  }

  const int is_cacheable_based_on_mmu_status =
      (dcache != nullptr) ? isCacheableRequestBasedOnMmuStatus(dcache, cpu_id) : 1;
  const uint8_t do_mmu_read_dword =
      (uint8_t) (is_read && (is_mmu_access || is_mmu_bypass || (lock_mask != 0) ||
                             !is_cacheable_based_on_mmu_status));
  const uint8_t do_mmu_write_dword =
      (uint8_t) ((!is_read) && (is_cached_mem_access || is_mmu_access || is_mmu_bypass ||
                                !is_cacheable_based_on_mmu_status));
  const uint8_t do_mmu_fetch_line =
      (uint8_t) (is_cached_mem_access && !is_hit && (lock_mask == 0) &&
                 is_cacheable_based_on_mmu_status);

  if (do_mmu_read_dword || do_mmu_write_dword) {
    const uint8_t cmd =
        (req == REQUEST_TYPE_WRFSRFAR)
            ? MMU_WRITE_FSR
            : (do_mmu_read_dword ? MMU_READ_DWORD
                                 : (is_hit ? MMU_WRITE_DWORD_NO_RESPONSE : MMU_WRITE_DWORD));

    uint64_t dummy = 0;
    uint64_t* mmu_rdata = do_mmu_read_dword ? read_data : &dummy;
    StepTaskT<void> mmu_task = Mmu(owner, ms, cpu_id,
                                   cmd,
                                   (uint8_t) (req | lock_mask),
                                   asi_7b,
                                   addr,
                                   byte_mask,
                                   write_data,
                                   &lmae, &cacheable, &acc,
                                   mmu_rdata,
                                   &mmu_fsr, &synonym);
    CO_AWAIT_OWNED(owner, mmu_task);
  }

  const uint8_t first_is_not_cacheable =
      (uint8_t) ((do_mmu_read_dword || do_mmu_write_dword) && !cacheable);
  if (!lmae && !first_is_not_cacheable && do_mmu_fetch_line) {
    uint64_t line_data[8] = {};
    StepTaskT<void> line_task = Mmu(owner, ms, cpu_id,
                                    MMU_READ_LINE,
                                    req,
                                    asi_7b,
                                    addr,
                                    byte_mask,
                                    write_data,
                                    &lmae, &cacheable, &acc,
                                    line_data,
                                    &mmu_fsr, &synonym);
    CO_AWAIT_OWNED(owner, line_task);

    if ((synonym != 0) && (dcache != nullptr)) {
      uint32_t invalidate_va = (synonym & 0x7fffffffu);
      invalidateCacheLine(dcache, invalidate_va);
    }

    if (!lmae) {
      // Mirror legacy D-cache behavior on miss refill: update cache-line when cacheable
      // regardless of whether caller requested read_data payload.
      if (cacheable && (dcache != nullptr)) {
        uint32_t line_addr = (addr & LINE_ADDR_MASK);
        updateCacheLine(dcache, acc, context, line_addr, line_data);
        if (read_data != nullptr) {
          *read_data = getDwordFromCache(dcache, context, addr);
        }
      } else if (read_data != nullptr) {
        *read_data = line_data[cacheLineOffset(addr)];
      }
    }
  }

  // Legacy D-cache hook: update MMU-facing cache state on MMU-control writes.
  if ((dcache != nullptr) && (asi_7b == 0x4u) && (req == REQUEST_TYPE_WRITE)) {
    updateMmuState(dcache, cpu_id, byte_mask, addr, write_data);
  }

  if (mae) *mae = lmae;
  if (lock_acquired) {
    StepTaskT<void> unl = unlock_or_yield_phase1(owner, &dcache->cache_mutex);
    CO_AWAIT_OWNED(owner, unl);
  }
  co_return;
}

extern "C" void ajit_step_get_contention_counters(uint64_t* icache_busy,
                                                   uint64_t* dcache_busy,
                                                   uint64_t* mmu_busy,
                                                   uint64_t* dcache_owner_busy)
{
  for (int c = 0; c < 4; ++c) {
    if (icache_busy != nullptr) {
      icache_busy[c] = g_lock_busy_counts[LOCK_COUNTER_ICACHE][c];
    }
    if (dcache_busy != nullptr) {
      dcache_busy[c] = g_lock_busy_counts[LOCK_COUNTER_DCACHE][c];
    }
    if (mmu_busy != nullptr) {
      mmu_busy[c] = g_lock_busy_counts[LOCK_COUNTER_MMU][c];
    }
    if (dcache_owner_busy != nullptr) {
      dcache_owner_busy[c] = g_dcache_owner_busy_counts[c];
    }
  }
}

extern "C" void ajit_step_get_mmu_source_counters(uint64_t* mmu_req_ifetch,
                                                   uint64_t* mmu_req_dcache,
                                                   uint64_t* mmu_busy_ifetch,
                                                   uint64_t* mmu_busy_dcache)
{
  for (int c = 0; c < 4; ++c) {
    if (mmu_req_ifetch != nullptr) {
      mmu_req_ifetch[c] = g_mmu_req_by_source[MMU_SOURCE_IFETCH][c];
    }
    if (mmu_req_dcache != nullptr) {
      mmu_req_dcache[c] = g_mmu_req_by_source[MMU_SOURCE_DCACHE][c];
    }
    if (mmu_busy_ifetch != nullptr) {
      mmu_busy_ifetch[c] = g_mmu_busy_by_source[MMU_SOURCE_IFETCH][c];
    }
    if (mmu_busy_dcache != nullptr) {
      mmu_busy_dcache[c] = g_mmu_busy_by_source[MMU_SOURCE_DCACHE][c];
    }
  }
}

extern "C" void ajit_step_get_contention_counters_by_phase(uint64_t* icache_busy_p0,
                                                            uint64_t* icache_busy_p1,
                                                            uint64_t* dcache_busy_p0,
                                                            uint64_t* dcache_busy_p1,
                                                            uint64_t* mmu_busy_p0,
                                                            uint64_t* mmu_busy_p1,
                                                            uint64_t* dcache_owner_busy_p0,
                                                            uint64_t* dcache_owner_busy_p1)
{
  for (int c = 0; c < 4; ++c) {
    if (icache_busy_p0 != nullptr) {
      icache_busy_p0[c] = g_lock_busy_counts_by_phase[LOCK_COUNTER_ICACHE][0][c];
    }
    if (icache_busy_p1 != nullptr) {
      icache_busy_p1[c] = g_lock_busy_counts_by_phase[LOCK_COUNTER_ICACHE][1][c];
    }
    if (dcache_busy_p0 != nullptr) {
      dcache_busy_p0[c] = g_lock_busy_counts_by_phase[LOCK_COUNTER_DCACHE][0][c];
    }
    if (dcache_busy_p1 != nullptr) {
      dcache_busy_p1[c] = g_lock_busy_counts_by_phase[LOCK_COUNTER_DCACHE][1][c];
    }
    if (mmu_busy_p0 != nullptr) {
      mmu_busy_p0[c] = g_lock_busy_counts_by_phase[LOCK_COUNTER_MMU][0][c];
    }
    if (mmu_busy_p1 != nullptr) {
      mmu_busy_p1[c] = g_lock_busy_counts_by_phase[LOCK_COUNTER_MMU][1][c];
    }
    if (dcache_owner_busy_p0 != nullptr) {
      dcache_owner_busy_p0[c] = g_dcache_owner_busy_counts_by_phase[0][c];
    }
    if (dcache_owner_busy_p1 != nullptr) {
      dcache_owner_busy_p1[c] = g_dcache_owner_busy_counts_by_phase[1][c];
    }
  }
}

extern "C" void ajit_step_get_mmu_source_counters_by_phase(uint64_t* mmu_busy_ifetch_p0,
                                                            uint64_t* mmu_busy_ifetch_p1,
                                                            uint64_t* mmu_busy_dcache_p0,
                                                            uint64_t* mmu_busy_dcache_p1)
{
  for (int c = 0; c < 4; ++c) {
    if (mmu_busy_ifetch_p0 != nullptr) {
      mmu_busy_ifetch_p0[c] = g_mmu_busy_by_source_phase[MMU_SOURCE_IFETCH][0][c];
    }
    if (mmu_busy_ifetch_p1 != nullptr) {
      mmu_busy_ifetch_p1[c] = g_mmu_busy_by_source_phase[MMU_SOURCE_IFETCH][1][c];
    }
    if (mmu_busy_dcache_p0 != nullptr) {
      mmu_busy_dcache_p0[c] = g_mmu_busy_by_source_phase[MMU_SOURCE_DCACHE][0][c];
    }
    if (mmu_busy_dcache_p1 != nullptr) {
      mmu_busy_dcache_p1[c] = g_mmu_busy_by_source_phase[MMU_SOURCE_DCACHE][1][c];
    }
  }
}
