#include "modular_icache_adapter.h"

extern "C" {
#include "MmuCommands.h"
#include "RequestTypeValues.h"
}

#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace {

constexpr int kMaxModularCores = 4;
ModularIcacheAdapter* g_icache_adapters[kMaxModularCores] = {};

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

void ModularIcacheAdapter::init(int core, int tpc)
{
  core_id = core;
  threads_per_core = std::max(1, std::min(tpc, 2));
  if (cache == nullptr) {
    const uint32_t lines = parse_env_u32_min1("AJIT_THREAD_ICACHE_NUMBER_OF_LINES", 512);
    const uint32_t assoc = parse_env_u32_min1("AJIT_THREAD_ICACHE_ASSOCIATIVITY", 1);
    cache = makeCache((uint32_t) core_id, 1, (int) lines, (int) assoc);
  }
  if (core_id >= 0 && core_id < kMaxModularCores) {
    g_icache_adapters[core_id] = this;
  }
}

void ModularIcacheAdapter::set_active(std::coroutine_handle<> h) noexcept
{
  active = h;
}

std::coroutine_handle<> ModularIcacheAdapter::get_active() const noexcept
{
  return active;
}

bool ModularIcacheAdapter::idle() const
{
  return !root || !root.h || root.h.done();
}

void ModularIcacheAdapter::dump_summary() const
{
  if (cache == nullptr) {
    return;
  }
  std::fprintf(stderr,
               "MOD-CACHE-SUMMARY c%d IF(a=%llu h=%llu m=%llu f=%llu)\n",
               core_id,
               (unsigned long long) cache->number_of_accesses,
               (unsigned long long) cache->number_of_hits,
               (unsigned long long) cache->number_of_misses,
               (unsigned long long) cache->number_of_flushes);
}

extern "C" void modular_icache_dump_summary(void)
{
  for (int core_id = 0; core_id < kMaxModularCores; ++core_id) {
    if (g_icache_adapters[core_id] != nullptr) {
      g_icache_adapters[core_id]->dump_summary();
    }
  }
}

extern "C" int modular_icache_get_counters(int core_id, uint64_t* accesses,
                                            uint64_t* misses, uint64_t* flushes)
{
  if (core_id < 0 || core_id >= kMaxModularCores ||
      g_icache_adapters[core_id] == nullptr || g_icache_adapters[core_id]->cache == nullptr) {
    return 0;
  }
  const WriteThroughAllocateCache* cache = g_icache_adapters[core_id]->cache;
  if (accesses) *accesses = cache->number_of_accesses;
  if (misses) *misses = cache->number_of_misses;
  if (flushes) *flushes = cache->number_of_flushes;
  return 1;
}

void ModularIcacheAdapter::run(uint64_t simulation_time)
{
  sim_time = simulation_time;
  auto h = get_active();
  if (h && !h.done()) {
    h.resume();
  }
}

bool ModularIcacheAdapter::try_accept_thread_request(int t)
{
  if (!idle() || t < 0 || t >= threads_per_core || t >= 2) {
    return false;
  }

  ModularThreadIcacheRequest& request = pending_thread_request[t];
  if (!modular_pull_thread_icache_request_step(&thread_pull_stage[t],
                                               thread_valid[t],
                                               thread_kind[t],
                                               thread_asi[t],
                                               thread_context[t],
                                               thread_addr[t],
                                               &request)) {
    return false;
  }

  active_thread = t;
  if (trace_adapter()) {
    std::fprintf(stderr,
                 "MOD-ICACHE c%d accepted t%d kind=%u addr=0x%08x\n",
                 core_id,
                 active_thread,
                 (unsigned) request.kind,
                 request.addr);
  }
  root = icache_access_coroutine(t, request);
  root.set_owner(this);
  set_active(root.h);
  return true;
}

StepTask ModularIcacheAdapter::wait_for_phase_0()
{
  while ((sim_time & 0x1ull) != 0) {
    co_await yield_point(this);
  }
}

StepTask ModularIcacheAdapter::wait_for_phase_1()
{
  while ((sim_time & 0x1ull) == 0) {
    co_await yield_point(this);
  }
}

StepTask ModularIcacheAdapter::push_thread_response(int thread_id, uint8_t mae, uint64_t data)
{
  const ModularResponse response = {
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

StepTaskT<ModularMmuResponse> ModularIcacheAdapter::mmu_access_over_nets(
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

StepTaskT<ModularMmuResponse> ModularIcacheAdapter::mmu_line_access_over_nets(
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

StepTask ModularIcacheAdapter::icache_access_coroutine(
    int thread_id,
    ModularThreadIcacheRequest request)
{
  uint8_t mae = 0;
  uint64_t instr_pair = 0;
  uint8_t context = request.context;
  const uint8_t asi_7b = (uint8_t) (request.asi & 0x7fu);
  const uint8_t req = (uint8_t) (request.kind & 0x3fu);

  while (cache != nullptr) {
    const uint32_t invalidate_word = probeCoherencyInvalidateRequest(core_id, 1);
    if (invalidate_word == 0) {
      break;
    }
    const uint32_t invalidate_va =
        (invalidate_word & 0x7fffffffu) << LOG_BYTES_PER_CACHE_LINE;
    invalidateCacheLine(cache, invalidate_va);
  }

  uint8_t is_nop = 0;
  uint8_t is_flush = 0;
  uint8_t is_ifetch = 0;
  decodeIcacheRequest(asi_7b, req, &is_nop, &is_flush, &is_ifetch);

  if (cache != nullptr) {
    cache->number_of_accesses++;
  }

  if (is_nop) {
    StepTask resp = push_thread_response(thread_id, 0, 0);
    CO_AWAIT_OWNED(this, resp);
    co_return;
  }

  if (is_ifetch) {
    if ((cache != nullptr) && !cache->multi_context) {
      context = 0;
    }

    uint8_t acc = 0;
    uint8_t is_hit = 0;
    if (cache != nullptr) {
      uint8_t is_raw_hit = 0;
      lookupCache(cache, context, request.addr, asi_7b, &is_raw_hit, &acc);
      const int access_ok = accessPermissionsOk(REQUEST_TYPE_IFETCH, asi_7b, acc);
      is_hit = (uint8_t) (is_raw_hit && access_ok);
    }

    if (is_hit) {
      if (cache != nullptr) {
        cache->number_of_hits++;
      }
      instr_pair = getDwordFromCache(cache, context, request.addr);
      mae = (uint8_t) ((1u << 7) | ((acc & 0x7u) << 4));
    } else {
      if (cache != nullptr) {
        cache->number_of_misses++;
      }

      uint8_t lmae = 0;
      uint8_t cacheable = 1;
      acc = 3;
      uint32_t synonym = 0;
      uint64_t line_data[8] = {};

      const ModularMmuRequest mmu_request = {
          0,
          (uint8_t) thread_id,
          MMU_READ_LINE,
          REQUEST_TYPE_IFETCH,
          asi_7b,
          context,
          request.addr,
          0,
          0xff,
      };
      StepTaskT<ModularMmuResponse> line_task =
          mmu_line_access_over_nets(mmu_request, line_data);
      line_task.set_owner(this);
      ModularMmuResponse line_response = co_await line_task;
      lmae = line_response.mae;
      cacheable = line_response.cacheable;
      acc = line_response.acc;
      synonym = line_response.synonym;

      if ((synonym != 0) && (cache != nullptr)) {
        const uint32_t invalidate_va =
            (synonym & 0x7fffffffu) << LOG_BYTES_PER_CACHE_LINE;
        invalidateCacheLine(cache, invalidate_va);
      }

      if (!lmae) {
        const int line_offset = cacheLineOffset(request.addr);
        if (cacheable && cache != nullptr) {
          const uint32_t line_addr = (request.addr & LINE_ADDR_MASK);
          updateCacheLine(cache, acc, context, line_addr, line_data);
          instr_pair = line_data[line_offset];
        } else {
          instr_pair = line_data[line_offset];
        }
        if (trace_adapter()) {
          std::fprintf(stderr,
                       "MOD-ICACHE miss-fill c%d t%d addr=0x%08x off=%d data=0x%016llx\n",
                       core_id,
                       thread_id,
                       request.addr,
                       line_offset,
                       (unsigned long long) instr_pair);
        }
      }
      mae = (uint8_t) (((cacheable & 0x1u) << 7) |
                       ((acc & 0x7u) << 4) |
                       (lmae & 0x3u));
    }

    StepTask resp = push_thread_response(thread_id, mae, instr_pair);
    CO_AWAIT_OWNED(this, resp);
    co_return;
  }

  if (is_flush) {
    if (cache != nullptr) {
      cache->number_of_flushes++;
      flushCache(cache);
    }
    StepTask resp = push_thread_response(thread_id, 0, 0);
    CO_AWAIT_OWNED(this, resp);
    co_return;
  }

  StepTask resp = push_thread_response(thread_id, 1, 0);
  CO_AWAIT_OWNED(this, resp);
}
