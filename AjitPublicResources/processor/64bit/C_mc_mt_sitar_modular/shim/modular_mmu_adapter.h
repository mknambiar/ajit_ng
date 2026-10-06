#pragma once

#include "coroutine_runtime.h"
#include "modular_packet_helpers.h"

#include <cstdint>

namespace sitar {
class module;
}

extern "C" {
#include "Mmu.h"
}

struct ModularMmuAdapter : public CoroutineOwner {
  int core_id = 0;

  void* icache_valid = nullptr;
  void* icache_source = nullptr;
  void* icache_thread_id = nullptr;
  void* icache_command = nullptr;
  void* icache_kind = nullptr;
  void* icache_asi = nullptr;
  void* icache_context = nullptr;
  void* icache_addr = nullptr;
  void* icache_data = nullptr;
  void* icache_byte_mask = nullptr;
  void* icache_resp_valid = nullptr;
  void* icache_resp_mae = nullptr;
  void* icache_resp_data = nullptr;
  void* icache_resp_cacheable = nullptr;
  void* icache_resp_acc = nullptr;
  void* icache_resp_fsr = nullptr;
  void* icache_resp_synonym = nullptr;
  void* icache_resp_meta = nullptr;

  void* dcache_valid = nullptr;
  void* dcache_source = nullptr;
  void* dcache_thread_id = nullptr;
  void* dcache_command = nullptr;
  void* dcache_kind = nullptr;
  void* dcache_asi = nullptr;
  void* dcache_context = nullptr;
  void* dcache_addr = nullptr;
  void* dcache_data = nullptr;
  void* dcache_byte_mask = nullptr;
  void* dcache_resp_valid = nullptr;
  void* dcache_resp_mae = nullptr;
  void* dcache_resp_data = nullptr;
  void* dcache_resp_cacheable = nullptr;
  void* dcache_resp_acc = nullptr;
  void* dcache_resp_fsr = nullptr;
  void* dcache_resp_synonym = nullptr;
  void* dcache_resp_meta = nullptr;

  void* bridge_valid = nullptr;
  void* bridge_write = nullptr;
  void* bridge_addr = nullptr;
  void* bridge_data = nullptr;
  void* bridge_byte_mask = nullptr;
  void* bridge_resp_data = nullptr;

  void init(int core);
  void bind_testbench_bridge_ports(sitar::module* mmu_module);
  bool idle() const;
  bool try_accept_client_request(int client_id);
  void run(uint64_t simulation_time);
  void dump_summary() const;
  void set_active(std::coroutine_handle<> h) noexcept override;
  std::coroutine_handle<> get_active() const noexcept override;

private:
  MmuState* mmu_state = nullptr;
  StepTaskT<ModularMmuResponse> root;
  std::coroutine_handle<> active = {};
  uint64_t sim_time = 0;
  int active_client = 0;
  uint8_t client_pull_stage[2] = {};
  ModularMmuRequest pending_client_request[2] = {};
  uint64_t client_requests[2] = {};
  uint64_t read_line_requests[2] = {};
  uint64_t read_dword_requests[2] = {};
  uint64_t write_dword_requests[2] = {};
  uint64_t write_dword_no_response_requests[2] = {};
  uint64_t write_fsr_requests[2] = {};
  uint64_t bridge_accesses[2] = {};
  uint64_t bridge_reads[2] = {};
  uint64_t bridge_writes[2] = {};
  uint64_t bridge_line_words[2] = {};

  StepTask wait_for_phase_0();
  StepTask wait_for_phase_1();
  StepTaskT<ModularResponse> bridge_access_over_nets(const ModularBridgeRequest& request);
  StepTaskT<int> read_pt_entry(uint8_t lock_mask, uint8_t thread_id, uint64_t physical_addr, uint32_t* pte);
  StepTask write_pt_entry(uint8_t lock_mask, uint8_t thread_id, uint64_t physical_addr, uint32_t pte);
  StepTaskT<int> walk_page_tables(uint8_t lock_mask,
                                  uint8_t thread_id,
                                  uint32_t virt_addr,
                                  uint32_t* pte,
                                  uint8_t* pte_level,
                                  uint64_t* phy_addr_of_pte);
  StepTaskT<int> perform_probe(uint8_t lock_mask, uint8_t thread_id, uint32_t virt_addr, uint8_t asi, uint8_t request_type, uint32_t* pte);
  StepTask push_client_response(int client_id, const ModularMmuResponse& response);
  StepTaskT<ModularMmuResponse> mmu_access_coroutine(ModularMmuRequest request);
};

extern "C" void modular_mmu_dump_summary(void);
