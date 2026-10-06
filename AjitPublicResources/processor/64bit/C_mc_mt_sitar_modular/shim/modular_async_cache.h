#pragma once
#include "modular_packet_helpers.h"
#include "sitar_net.h"
#include <cstdint>
extern "C" {
#include "CacheInterface.h"
}

// Native nets are declared by ICache/DCache, not hidden C++ ring buffers.
// Front branch exclusively owns cache mutations, counters and CPU responses.
// Worker branch exclusively owns MMU transactions. Shared cache reads by the
// worker occur only in phase 1, after front's phase-0 mutations.
struct ModularAsyncCache {
  void* thread_valid[2] = {};
  void* thread_kind[2] = {};
  void* thread_asi[2] = {};
  void* thread_context[2] = {};
  void* thread_addr[2] = {};
  void* thread_data[2] = {};
  void* thread_byte_mask[2] = {};
  void* thread_resp_valid[2] = {};
  void* thread_resp_mae[2] = {};
  void* thread_resp_data[2] = {};
  void* mmu_valid = nullptr;
  void* mmu_source = nullptr;
  void* mmu_thread_id = nullptr;
  void* mmu_command = nullptr;
  void* mmu_kind = nullptr;
  void* mmu_asi = nullptr;
  void* mmu_context = nullptr;
  void* mmu_addr = nullptr;
  void* mmu_data = nullptr;
  void* mmu_byte_mask = nullptr;
  void* mmu_resp_valid = nullptr;
  void* mmu_resp_mae = nullptr;
  void* mmu_resp_data = nullptr;
  void* mmu_resp_cacheable = nullptr;
  void* mmu_resp_acc = nullptr;
  void* mmu_resp_fsr = nullptr;
  void* mmu_resp_synonym = nullptr;
  void* mmu_resp_meta = nullptr;

  void init(WriteThroughAllocateCache*, int core, int threads, bool instruction,
            sitar::net<40>*, sitar::net<40>*, sitar::net<80>*);
  void front(uint64_t tick);
  void worker(uint64_t tick);
  void summary() const;

private:
  struct Work {
    uint8_t tid=0, flags=0;
    ModularThreadDcacheRequest req;
    uint32_t physical=0;
    uint64_t sequence=0, eligible=0;
  };
  enum Event : uint8_t { Hit, Dword, Write, Fill, Physical };
  struct Result {
    Work work;
    ModularMmuResponse response;
    Event event=Hit;
    bool first=true, terminal=true;
    uint64_t epoch=0;
  };
  struct Line {
    uint8_t words=0, tid=0, context=0;
    bool authorized=false;
    uint32_t physical=0, tag=0;
  };
  WriteThroughAllocateCache* cache=nullptr;
  int core=0, threads=1, next=0;
  bool instruction=false, trace=false, fault_trace=false;
  int atomic_owner=-1;
  sitar::net<40>* pending=nullptr;
  sitar::net<40>* direct=nullptr;
  sitar::net<80>* completed=nullptr;
  Line lines[MAX_NUMBER_OF_LINES] = {};
  uint64_t invalidate_epoch[MAX_NUMBER_OF_LINES] = {};
  uint8_t pull_stage[2]={}, response_stage[2]={};
  bool held[2]={}, unanswered[2]={}, response_ready[2]={};
  Work accepted[2];
  ModularThreadIcacheRequest irequest[2];
  ModularResponse responses[2];
  uint64_t serial=0, fence=0, outstanding=0;
  int fill_slot=-1;
  uint64_t fill_sequence=0, fill_epoch=0;
  uint32_t fill_tag=0;
  uint64_t enqueued=0, drained=0, full_stalls=0, ordering_stalls=0;
  uint64_t peak=0, fast_hits=0, critical_responses=0, faults=0;
  uint64_t forwarded=0, stores_pending=0, queued=0, bypassed=0;

  // Front-owned reservations, bounded by the two held CPU request slots.
  // Credits are sampled after phase-1 publication; worker pulls only in phase 0.
  Work publish_work[2];
  bool publish_direct[2]={};
  unsigned publish_count=0, pending_credit=4, direct_credit=1;

  // Worker-owned state. Front never reads these fields.
  Work dispatch_work;
  bool dispatch_ready=false;
  bool busy=false, sent=false, got=false, have_result=false;
  bool refill_after_write=false, first_beat=true;
  uint8_t request_stage=0, mmu_stage=0;
  Work current;
  ModularMmuRequest request;
  ModularMmuResponse response;
  Result result;
  Event operation=Hit;
  uint64_t transaction_epoch=0;

  static sitar::token<40> pack(const Work&);
  static Work unpack(sitar::token<40>&);
  static sitar::token<80> pack_result(const Result&);
  static Result unpack_result(sitar::token<80>&);
  uint8_t context(const Work&) const;
  bool cached(const Work&) const;
  bool ordered(const Work&) const;
  bool read(const Work&) const;
  int hit(const Work&) const;
  bool authorize(const Work&, int) const;
  void respond(const Work&, uint8_t, uint64_t);
  void accept_result(const Result&);
  void invalidate(uint32_t va_line);
  void flush();
  void count(const Work&, bool);
  bool enqueue(Work&, uint64_t);
  void start(const Work&);
  void prepare_mmu(uint8_t command);
};
extern "C" void modular_async_cache_dump_summary();

extern "C" int modular_async_buffered_stores_pending();
