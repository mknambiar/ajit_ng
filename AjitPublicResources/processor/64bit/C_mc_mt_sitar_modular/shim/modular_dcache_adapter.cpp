#include "modular_dcache_adapter.h"

extern "C" {
#include "MmuCommands.h"
#include "RequestTypeValues.h"
}

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <utility>

namespace {

constexpr int kMaxModularCores = 4;
ModularDcacheAdapter* g_dcache_adapters[kMaxModularCores] = {};

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

} // namespace

void ModularDcacheAdapter::init(int core, int tpc)
{
  core_id = core;
  threads_per_core = std::max(1, std::min(tpc, 2));
  if (cache == nullptr) {
    const uint32_t lines = parse_env_u32_min1("AJIT_THREAD_DCACHE_NUMBER_OF_LINES", 512);
    const uint32_t assoc = parse_env_u32_min1("AJIT_THREAD_DCACHE_ASSOCIATIVITY", 1);
    cache = makeCache((uint32_t) core_id, 0, (int) lines, (int) assoc);
  }
  if (core_id >= 0 && core_id < kMaxModularCores) {
    g_dcache_adapters[core_id] = this;
  }
}

void ModularDcacheAdapter::set_active(std::coroutine_handle<> h) noexcept
{
  active = h;
}

std::coroutine_handle<> ModularDcacheAdapter::get_active() const noexcept
{
  return active;
}

bool ModularDcacheAdapter::idle() const
{
  return !root || !root.h || root.h.done();
}

void ModularDcacheAdapter::dump_summary() const
{
  if (cache == nullptr) {
    return;
  }
  std::fprintf(stderr,
               "MOD-CACHE-SUMMARY c%d DF(a=%llu h=%llu m=%llu rh=%llu rm=%llu wh=%llu wm=%llu by=%llu fl=%llu lk=%u)\n",
               core_id,
               (unsigned long long) cache->number_of_accesses,
               (unsigned long long) cache->number_of_hits,
               (unsigned long long) cache->number_of_misses,
               (unsigned long long) cache->number_of_read_hits,
               (unsigned long long) cache->number_of_read_misses,
               (unsigned long long) cache->number_of_write_hits,
               (unsigned long long) cache->number_of_write_misses,
               (unsigned long long) cache->number_of_bypasses,
               (unsigned long long) cache->number_of_flushes,
               cache->number_of_locked_accesses);
}

extern "C" void modular_dcache_dump_summary(void)
{
  for (int core_id = 0; core_id < kMaxModularCores; ++core_id) {
    if (g_dcache_adapters[core_id] != nullptr) {
      g_dcache_adapters[core_id]->dump_summary();
    }
  }
}

extern "C" int modular_dcache_get_counters(int core_id, uint64_t* accesses,
                                            uint64_t* misses, uint64_t* read_misses,
                                            uint64_t* write_misses, uint64_t* flushes)
{
  if (core_id < 0 || core_id >= kMaxModularCores ||
      g_dcache_adapters[core_id] == nullptr || g_dcache_adapters[core_id]->cache == nullptr) {
    return 0;
  }
  const WriteThroughAllocateCache* cache = g_dcache_adapters[core_id]->cache;
  if (accesses) *accesses = cache->number_of_accesses;
  if (misses) *misses = cache->number_of_misses;
  if (read_misses) *read_misses = cache->number_of_read_misses;
  if (write_misses) *write_misses = cache->number_of_write_misses;
  if (flushes) *flushes = cache->number_of_flushes;
  return 1;
}

void ModularDcacheAdapter::run(uint64_t simulation_time)
{
  sim_time = simulation_time;
  auto h = get_active();
  if (h && !h.done()) {
    h.resume();
  }
}

bool ModularDcacheAdapter::try_accept_thread_request(int t)
{
  if (!idle() || t < 0 || t >= threads_per_core || t >= 2) {
    return false;
  }

  ModularThreadDcacheRequest& request = pending_thread_request[t];
  if (!modular_pull_thread_dcache_request_step(&thread_pull_stage[t],
                                               thread_valid[t],
                                               thread_kind[t],
                                               thread_asi[t],
                                               thread_context[t],
                                               thread_addr[t],
                                               thread_data[t],
                                               thread_byte_mask[t],
                                               &request)) {
    return false;
  }

  active_thread = t;
  if (trace_adapter()) {
    std::fprintf(stderr,
                 "MOD-DCACHE c%d accepted t%d kind=%u addr=0x%08x\n",
                 core_id,
                 active_thread,
                 (unsigned) request.kind,
                 request.addr);
  }

  root = dcache_access_coroutine(t, request);
  root.set_owner(this);
  set_active(root.h);
  return true;
}

StepTask ModularDcacheAdapter::wait_for_phase_0()
{
  while ((sim_time & 0x1ull) != 0) {
    co_await yield_point(this);
  }
}

StepTask ModularDcacheAdapter::wait_for_phase_1()
{
  while ((sim_time & 0x1ull) == 0) {
    co_await yield_point(this);
  }
}

StepTask ModularDcacheAdapter::push_thread_response(int thread_id, uint8_t mae, uint64_t data)
{
  ModularResponse response = {
      mae,
      data,
  };
  uint8_t resp_stage = 0;
  while (true) {
    StepTask p1 = wait_for_phase_1();
    CO_AWAIT_OWNED(this, p1);
    if (modular_push_response_step(&resp_stage,
                                   thread_resp_valid[thread_id],
                                   thread_resp_mae[thread_id],
                                   thread_resp_data[thread_id],
                                   response)) {
      co_return;
    }
    co_await yield_point(this);
  }
}

StepTaskT<ModularMmuResponse> ModularDcacheAdapter::mmu_access_over_nets(
    const ModularMmuRequest& request)
{
  uint8_t req_stage = 0;
  while (true) {
    StepTask p1 = wait_for_phase_1();
    CO_AWAIT_OWNED(this, p1);
    if (modular_push_mmu_request_step(&req_stage,
                                      mmu_valid,
                                      mmu_source,
                                      mmu_thread_id,
                                      mmu_command,
                                      mmu_kind,
                                      mmu_asi,
                                      mmu_context,
                                      mmu_addr,
                                      mmu_data,
                                      mmu_byte_mask,
                                      request)) {
      break;
    }
    co_await yield_point(this);
  }

  ModularMmuResponse response;
  uint8_t resp_stage = 0;
  while (true) {
    StepTask p0 = wait_for_phase_0();
    CO_AWAIT_OWNED(this, p0);
    if (modular_pull_mmu_response_step(&resp_stage,
                                       mmu_resp_valid,
                                       mmu_resp_mae,
                                       mmu_resp_data,
                                       mmu_resp_cacheable,
                                       mmu_resp_acc,
                                       mmu_resp_fsr,
                                       mmu_resp_synonym,
                                       &response)) {
      co_return response;
    }
    co_await yield_point(this);
  }
}

StepTaskT<ModularMmuResponse> ModularDcacheAdapter::mmu_line_access_over_nets(
    const ModularMmuRequest& request,
    uint64_t line_data[8])
{
  uint8_t req_stage = 0;
  while (true) {
    StepTask p1 = wait_for_phase_1();
    CO_AWAIT_OWNED(this, p1);
    if (modular_push_mmu_request_step(&req_stage,
                                      mmu_valid,
                                      mmu_source,
                                      mmu_thread_id,
                                      mmu_command,
                                      mmu_kind,
                                      mmu_asi,
                                      mmu_context,
                                      mmu_addr,
                                      mmu_data,
                                      mmu_byte_mask,
                                      request)) {
      break;
    }
    co_await yield_point(this);
  }

  ModularMmuResponse response;
  uint8_t resp_stage = 0;
  for (int i = 0; i < 8; ++i) {
    while (true) {
      StepTask p0 = wait_for_phase_0();
      CO_AWAIT_OWNED(this, p0);
      if (modular_pull_mmu_response_step(&resp_stage,
                                         mmu_resp_valid,
                                         mmu_resp_mae,
                                         mmu_resp_data,
                                         mmu_resp_cacheable,
                                         mmu_resp_acc,
                                         mmu_resp_fsr,
                                         mmu_resp_synonym,
                                         &response)) {
        line_data[i] = response.data;
        if (response.mae != 0) {
          co_return response;
        }
        break;
      }
      co_await yield_point(this);
    }
  }
  co_return response;
}

StepTask ModularDcacheAdapter::dcache_access_coroutine(
    int thread_id,
    ModularThreadDcacheRequest request)
{
  uint8_t lmae = 0;
  uint64_t local_read_data = 0;
  uint8_t cacheable = 0;
  uint8_t acc = 0;
  uint32_t mmu_fsr = 0;
  uint32_t synonym = 0;
  (void) mmu_fsr;

  const uint8_t lock_mask = (uint8_t) (request.kind & 0x40u);
  uint8_t req = (uint8_t) (request.kind & 0x3fu);
  const uint8_t asi_7b = (uint8_t) (request.asi & 0x7fu);
  uint8_t context = request.context;

  if ((cache != nullptr) && !cache->multi_context) {
    context = 0;
  }

  if (req == REQUEST_TYPE_CCU_CACHE_READ) {
    req = REQUEST_TYPE_READ;
  } else if (req == REQUEST_TYPE_CCU_CACHE_WRITE) {
    req = REQUEST_TYPE_WRITE;
  }

  if (cache != nullptr) {
    cache->number_of_accesses++;
    if (lock_mask != 0) {
      cache->number_of_locked_accesses++;
      cache->lock_flag = 1;
      cache->lock_core_id = (uint8_t) core_id;
    } else {
      cache->lock_flag = 0;
    }
  }

  while (cache != nullptr) {
    const uint32_t invalidate_word = probeCoherencyInvalidateRequest(core_id, 0);
    if (invalidate_word == 0) {
      break;
    }
    invalidateCacheLine(cache, (invalidate_word & 0x7fffffffu));
  }

  if ((asi_7b >= 0x10u) && (asi_7b <= 0x14u)) {
    if (cache != nullptr) {
      cache->number_of_flushes++;
      flushCache(cache);
    }
    StepTask resp = push_thread_response(thread_id, 0, 0);
    CO_AWAIT_OWNED(this, resp);
    co_return;
  }

  uint8_t is_nop = 0;
  uint8_t is_stbar = 0;
  uint8_t is_flush = 0;
  uint8_t is_read = 0;
  uint8_t is_cached_mem_access = 0;
  uint8_t is_mmu_access = 0;
  uint8_t is_mmu_bypass = 0;
  decodeDcacheRequest(asi_7b,
                      req,
                      &is_nop,
                      &is_stbar,
                      &is_flush,
                      &is_read,
                      &is_cached_mem_access,
                      &is_mmu_access,
                      &is_mmu_bypass);
  (void) is_flush;

  if (is_nop) {
    StepTask resp = push_thread_response(thread_id, 0, 0);
    CO_AWAIT_OWNED(this, resp);
    co_return;
  }

  if (is_mmu_access && (cache != nullptr)) {
    cache->number_of_flushes++;
    if (!cache->multi_context || !isMmuContextPointerWrite(asi_7b, request.addr)) {
      flushCache(cache);
    }
  }

  const uint8_t is_supported_req =
      (req == REQUEST_TYPE_READ) ||
      (req == REQUEST_TYPE_WRITE) ||
      (req == REQUEST_TYPE_WRFSRFAR) ||
      (req == REQUEST_TYPE_STBAR);
  if (!is_supported_req) {
    StepTask resp = push_thread_response(thread_id, 1, 0);
    CO_AWAIT_OWNED(this, resp);
    co_return;
  }

  uint8_t is_hit = 0;
  if ((cache != nullptr) && is_cached_mem_access) {
    uint8_t is_raw_hit = 0;
    lookupCache(cache, context, request.addr, asi_7b, &is_raw_hit, &acc);
    const int access_ok = accessPermissionsOk(req, asi_7b, acc);
    is_hit = (uint8_t) (is_raw_hit && access_ok);

    if (is_hit && is_read) {
      cache->number_of_hits++;
      cache->number_of_read_hits++;
      local_read_data = getDwordFromCache(cache, context, request.addr);
    }
    if (is_hit && !is_read) {
      cache->number_of_hits++;
      cache->number_of_write_hits++;
      writeIntoLine(cache, request.data, context, request.addr, request.byte_mask);
    }
    if (!is_hit && is_read) {
      cache->number_of_misses++;
      cache->number_of_read_misses++;
    }
    if (!is_hit && !is_read) {
      cache->number_of_misses++;
      cache->number_of_write_misses++;
    }
  } else if (!is_mmu_access && (cache != nullptr)) {
    cache->number_of_bypasses++;
  }

  if (req == REQUEST_TYPE_STBAR) {
    StepTask resp = push_thread_response(thread_id, 0, local_read_data);
    CO_AWAIT_OWNED(this, resp);
    co_return;
  }

  const int is_cacheable_based_on_mmu_status =
      (cache != nullptr) ? isCacheableRequestBasedOnMmuStatus(cache, thread_id) : 1;
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
    (void) cmd;
    const ModularMmuRequest mmu_request = {
        1,
        (uint8_t) thread_id,
        cmd,
        (uint8_t) (req | lock_mask),
        asi_7b,
        context,
        request.addr,
        request.data,
        request.byte_mask,
    };
    StepTaskT<ModularMmuResponse> mmu_task = mmu_access_over_nets(mmu_request);
    mmu_task.set_owner(this);
    ModularMmuResponse mmu_response = co_await mmu_task;
    lmae = mmu_response.mae;
    cacheable = mmu_response.cacheable;
    acc = mmu_response.acc;
    mmu_fsr = mmu_response.mmu_fsr;
    synonym = mmu_response.synonym;
    if (do_mmu_read_dword) {
      local_read_data = mmu_response.data;
    }
  }

  const uint8_t first_is_not_cacheable =
      (uint8_t) ((do_mmu_read_dword || do_mmu_write_dword) && !cacheable);
  if (!lmae && !first_is_not_cacheable && do_mmu_fetch_line) {
    uint64_t line_data[8] = {};
    const ModularMmuRequest mmu_request = {
        1,
        (uint8_t) thread_id,
        MMU_READ_LINE,
        req,
        asi_7b,
        context,
        request.addr,
        request.data,
        request.byte_mask,
    };
    StepTaskT<ModularMmuResponse> line_task =
        mmu_line_access_over_nets(mmu_request, line_data);
    line_task.set_owner(this);
    ModularMmuResponse line_response = co_await line_task;
    lmae = line_response.mae;
    cacheable = line_response.cacheable;
    acc = line_response.acc;
    mmu_fsr = line_response.mmu_fsr;
    synonym = line_response.synonym;

    if ((synonym != 0) && (cache != nullptr)) {
      invalidateCacheLine(cache, (synonym & 0x7fffffffu));
    }

    if (!lmae) {
      if (cache != nullptr) {
        const uint32_t line_addr = (request.addr & LINE_ADDR_MASK);
        updateCacheLine(cache, acc, context, line_addr, line_data);
        local_read_data = getDwordFromCache(cache, context, request.addr);
      } else {
        local_read_data = line_data[cacheLineOffset(request.addr)];
      }
    }
  }

  if ((cache != nullptr) && (asi_7b == 0x4u) && (req == REQUEST_TYPE_WRITE)) {
    updateMmuState(cache, thread_id, request.byte_mask, request.addr, request.data);
  }

  StepTask resp = push_thread_response(thread_id, lmae, local_read_data);
  CO_AWAIT_OWNED(this, resp);
}
