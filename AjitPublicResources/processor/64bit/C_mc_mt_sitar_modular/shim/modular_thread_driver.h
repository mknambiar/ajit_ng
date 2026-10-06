#pragma once

#include "coroutine_runtime.h"

#include <cstdint>
#include <coroutine>

namespace sitar {
class module;
}

struct ModularAjitThreadDriver : public CoroutineOwner {
  int slot_id = 0;
  uint64_t sim_time = 0;
  int owner_tid = -1;
  std::coroutine_handle<> active{};
  StepTask root;

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
  void* coh_fill_kind = nullptr;
  void* coh_fill_pa_line = nullptr;
  void* coh_fill_va_line = nullptr;
  void* coh_icache_inval = nullptr;
  void* coh_dcache_inval = nullptr;

  void set_active(std::coroutine_handle<> h) noexcept override;
  std::coroutine_handle<> get_active() const noexcept override;

  void bind_testbench_boundary_ports(sitar::module* thread_module);
  void register_ports();
  void run(uint64_t simulation_time);

private:
  StepTask driver();
  StepTaskT<int> thread_step();
};
