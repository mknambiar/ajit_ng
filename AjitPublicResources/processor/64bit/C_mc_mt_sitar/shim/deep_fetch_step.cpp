#include "deep_fetch_step.h"

#include "ajit_memory_api_shim.h"
#include "ajit_thread_bridge.h"
#include <cstdio>
#include <cstdlib>

namespace {

constexpr int kMaxTb = 8;
constexpr int kMaxCore = 4;

enum RequestType : uint8_t {
  REQUEST_TYPE_IFETCH = 0,
  REQUEST_TYPE_READ = 1,
  REQUEST_TYPE_WRITE = 2,
  REQUEST_TYPE_WRFSRFAR = 4,
  REQUEST_TYPE_CCU_CACHE_READ = 5,
  REQUEST_TYPE_CCU_CACHE_WRITE = 6
};

inline int get_threads_per_core() {
  return ajit_thread_bridge_get_threads_per_core();
}

inline int map_slot(int core_id, int thread_id) {
  int slot = (core_id * get_threads_per_core()) + thread_id;
  if (slot < 0 || slot >= kMaxTb) {
    std::fprintf(stderr,
                 "DEEP-BUS-ERR invalid slot mapping c%d t%d -> slot=%d\n",
                 core_id, thread_id, slot);
    return -1;
  }
  return slot;
}

inline bool is_read_req(uint8_t request_type) {
  const uint8_t rt = request_type & 0x3f;
  return (rt == REQUEST_TYPE_IFETCH) ||
         (rt == REQUEST_TYPE_READ) ||
         (rt == REQUEST_TYPE_CCU_CACHE_READ);
}

inline bool is_write_req(uint8_t request_type) {
  const uint8_t rt = request_type & 0x3f;
  return (rt == REQUEST_TYPE_WRITE) ||
         (rt == REQUEST_TYPE_WRFSRFAR) ||
         (rt == REQUEST_TYPE_CCU_CACHE_WRITE);
}

inline bool is_mmio_addr(uint32_t addr) {
  return (addr >= 0xffff0000u);
}

static volatile uint64_t g_bus_begin_read[kMaxTb] = {};
static volatile uint64_t g_bus_begin_write[kMaxTb] = {};
static volatile uint64_t g_bus_wait_read[kMaxTb] = {};
static volatile uint64_t g_bus_wait_write[kMaxTb] = {};
static volatile uint64_t g_bus_end_read[kMaxTb] = {};
static volatile uint64_t g_bus_end_write[kMaxTb] = {};

inline bool deepBusTraceEnabled() {
  static int init = 0;
  static bool enabled = false;
  if (!init) {
    const char* env = std::getenv("AJIT_TRACE_DEEP_BUS");
    enabled = (env && env[0] && (env[0] != '0'));
    init = 1;
  }
  return enabled;
}

}  // namespace

extern "C" uint64_t ajit_sitar_sim_time;

StepTaskT<int> sysMemBusRequest(CoroutineOwner* owner,
                                int core_id,
                                int thread_id,
                                uint8_t request_type,
                                uint8_t byte_mask,
                                uint32_t addr,
                                uint64_t data64,
                                uint64_t* rdata) {
  const bool trace = deepBusTraceEnabled();
  const int slot = map_slot(core_id, thread_id);
  if (slot < 0) {
    if (rdata) {
      *rdata = 0;
    }
    co_return 0;
  }
  const uint32_t bus_addr = is_mmio_addr(addr) ? addr : (addr & 0xfffffff8u);
  if (trace) {
    if (is_write_req(request_type)) {
      std::fprintf(stderr,
                   "DEEP-BUS-BEG c%d t%d slot=%d sim=%llu ph=%llu rt=0x%x addr=0x%08x bm=0x%02x wdata=0x%016llx\n",
                   core_id, thread_id, slot,
                   (unsigned long long) ajit_sitar_sim_time,
                   (unsigned long long) (ajit_sitar_sim_time & 0x1ull),
                   (unsigned) request_type, bus_addr,
                   (unsigned) byte_mask, (unsigned long long) data64);
    } else {
      std::fprintf(stderr,
                   "DEEP-BUS-BEG c%d t%d slot=%d sim=%llu ph=%llu rt=0x%x addr=0x%08x\n",
                   core_id, thread_id, slot,
                   (unsigned long long) ajit_sitar_sim_time,
                   (unsigned long long) (ajit_sitar_sim_time & 0x1ull),
                   (unsigned) request_type, bus_addr);
    }
  }

  if (is_read_req(request_type)) {
    __sync_fetch_and_add(&g_bus_begin_read[slot], 1ULL);
    uint64_t rd = 0;
    ajit_api_accessMemU64_begin(slot, 1, 0xff, bus_addr, 0);
    while (!ajit_api_accessMemU64(slot, ajit_sitar_sim_time, &rd)) {
      __sync_fetch_and_add(&g_bus_wait_read[slot], 1ULL);
      if (trace) {
        std::fprintf(stderr,
                     "DEEP-BUS-WAIT-READ slot=%d sim=%llu ph=%llu\n",
                     slot,
                     (unsigned long long) ajit_sitar_sim_time,
                     (unsigned long long) (ajit_sitar_sim_time & 0x1ull));
      }
      co_await yield_point(owner);
    }
    if (rdata) {
      *rdata = rd;
    }
    __sync_fetch_and_add(&g_bus_end_read[slot], 1ULL);
    if (trace) {
      std::fprintf(stderr, "DEEP-BUS-END-READ slot=%d data=0x%016llx\n",
                   slot, (unsigned long long) rd);
    }
    co_return 1;
  }

  if (is_write_req(request_type)) {
    __sync_fetch_and_add(&g_bus_begin_write[slot], 1ULL);
    ajit_api_accessMemU64_begin(slot, 0, byte_mask, bus_addr, data64);
    while (!ajit_api_accessMemU64(slot, ajit_sitar_sim_time, nullptr)) {
      __sync_fetch_and_add(&g_bus_wait_write[slot], 1ULL);
      if (trace) {
        std::fprintf(stderr,
                     "DEEP-BUS-WAIT-WRITE slot=%d sim=%llu ph=%llu\n",
                     slot,
                     (unsigned long long) ajit_sitar_sim_time,
                     (unsigned long long) (ajit_sitar_sim_time & 0x1ull));
      }
      co_await yield_point(owner);
    }
    if (rdata) {
      *rdata = 0;
    }
    __sync_fetch_and_add(&g_bus_end_write[slot], 1ULL);
    if (trace) {
      std::fprintf(stderr, "DEEP-BUS-END-WRITE slot=%d addr=0x%08x bm=0x%02x wdata=0x%016llx\n",
                   slot, bus_addr, (unsigned) byte_mask, (unsigned long long) data64);
    }
    co_return 1;
  }

  if (rdata) {
    *rdata = 0;
  }
  co_return 1;
}

extern "C" void ajit_step_dump_deep_bus_summary(void)
{
  uint64_t core_begin_read[kMaxCore] = {};
  uint64_t core_begin_write[kMaxCore] = {};
  uint64_t core_wait_read[kMaxCore] = {};
  uint64_t core_wait_write[kMaxCore] = {};
  uint64_t core_end_read[kMaxCore] = {};
  uint64_t core_end_write[kMaxCore] = {};

  const int threads_per_core = get_threads_per_core();
  const int slots_per_core = (threads_per_core > 0) ? threads_per_core : 1;

  for (int slot = 0; slot < kMaxTb; ++slot) {
    const int core = (slot / slots_per_core);
    const uint64_t br = g_bus_begin_read[slot];
    const uint64_t bw = g_bus_begin_write[slot];
    const uint64_t wr = g_bus_wait_read[slot];
    const uint64_t ww = g_bus_wait_write[slot];
    const uint64_t er = g_bus_end_read[slot];
    const uint64_t ew = g_bus_end_write[slot];

    std::fprintf(stderr,
                 "BRIDGE-BUS-SLOT s%d c%d begin-r=%llu begin-w=%llu wait-r=%llu wait-w=%llu end-r=%llu end-w=%llu\n",
                 slot, core,
                 (unsigned long long) br,
                 (unsigned long long) bw,
                 (unsigned long long) wr,
                 (unsigned long long) ww,
                 (unsigned long long) er,
                 (unsigned long long) ew);

    if ((core >= 0) && (core < kMaxCore)) {
      core_begin_read[core] += br;
      core_begin_write[core] += bw;
      core_wait_read[core] += wr;
      core_wait_write[core] += ww;
      core_end_read[core] += er;
      core_end_write[core] += ew;
    }
  }

  for (int core = 0; core < kMaxCore; ++core) {
    std::fprintf(stderr,
                 "BRIDGE-BUS-CORE c%d begin-r=%llu begin-w=%llu wait-r=%llu wait-w=%llu end-r=%llu end-w=%llu\n",
                 core,
                 (unsigned long long) core_begin_read[core],
                 (unsigned long long) core_begin_write[core],
                 (unsigned long long) core_wait_read[core],
                 (unsigned long long) core_wait_write[core],
                 (unsigned long long) core_end_read[core],
                 (unsigned long long) core_end_write[core]);
  }
}
