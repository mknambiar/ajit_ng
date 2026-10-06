#pragma once

#include "coroutine_runtime.h"
#include "modular_packet_helpers.h"

#include <cstdint>

extern "C" {
#include "CacheInterface.h"
}

struct ModularIcacheAdapter : public CoroutineOwner {
  int core_id = 0;
  int threads_per_core = 1;

  void* thread_valid[2] = {};
  void* thread_kind[2] = {};
  void* thread_asi[2] = {};
  void* thread_context[2] = {};
  void* thread_addr[2] = {};
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

  void init(int core, int tpc);
  bool idle() const;
  bool try_accept_thread_request(int thread_id);
  void run(uint64_t simulation_time);
  void dump_summary() const;
  void set_active(std::coroutine_handle<> h) noexcept override;
  std::coroutine_handle<> get_active() const noexcept override;

public:
  WriteThroughAllocateCache* cache = nullptr;
private:
  StepTask root;
  std::coroutine_handle<> active = {};
  uint64_t sim_time = 0;
  int active_thread = 0;
  uint8_t thread_pull_stage[2] = {};
  ModularThreadIcacheRequest pending_thread_request[2] = {};

  StepTask wait_for_phase_0();
  StepTask wait_for_phase_1();
  StepTask push_thread_response(int thread_id, uint8_t mae, uint64_t data);
  StepTaskT<ModularMmuResponse> mmu_access_over_nets(const ModularMmuRequest& request);
  StepTaskT<ModularMmuResponse> mmu_line_access_over_nets(const ModularMmuRequest& request,
                                                          uint64_t line_data[8]);
  StepTask icache_access_coroutine(int thread_id, ModularThreadIcacheRequest request);
};

extern "C" void modular_icache_dump_summary(void);
extern "C" int modular_icache_get_counters(int core_id, uint64_t* accesses,
                                            uint64_t* misses, uint64_t* flushes);
