#pragma once
#include "coroutine_runtime.h"
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <omp.h>
#include "ajit_memory_api_shim.h"
#include "ajit_thread_bridge.h"

extern "C" uint64_t ajit_sitar_sim_time;
extern "C" void ajit_shim_sample_irq(int id);

namespace sitar {

struct AjitTbDriver : public CoroutineOwner {
  int id = 0;
  uint64_t sim_time = 0;
  logger log;
  int owner_tid = -1;
  std::coroutine_handle<> active{};
  StepTask root;

  void set_active(std::coroutine_handle<> h) noexcept override { active = h; }
  std::coroutine_handle<> get_active() const noexcept override { return active; }

  StepTask wait_for_phase_1() {
    if (sim_time & 0x1) {
      co_return;
    }
    co_await yield_point(this);
    co_return;
  }

  StepTask wait_for_phase_0() {
    if ((sim_time & 0x1) == 0) {
      co_return;
    }
    co_await yield_point(this);
    co_return;
  }

  StepTaskT<int> thread_step() {
    StepTaskT<int> bridge_step = ajit_thread_bridge_step_task(this, id, sim_time);
    bridge_step.set_owner(this);
    int rc = co_await bridge_step;
    co_return rc;
  }

  StepTask driver() {
    while (true) {
      if ((sim_time & 0x1ull) != 0ull) {
        co_await yield_point(this);
        continue;
      }
      StepTaskT<int> t = thread_step();
      t.set_owner(this);
      (void) co_await t;
      co_await yield_point(this);
    }
  }

  void run(uint64_t simulation_time) {
    int tid = omp_get_thread_num();
    if (owner_tid < 0) owner_tid = tid;
    else if (owner_tid != tid) {
      std::fprintf(stderr,
                   "ERROR: AjitTbDriver %d migrated OpenMP threads! was %d now %d\n",
                   id, owner_tid, tid);
      std::abort();
    }
    sim_time = simulation_time;
    ajit_sitar_sim_time = simulation_time;
    ajit_shim_sample_irq(id);

    if (!root) {
      root = driver();
      root.set_owner(this);
      set_active(root.h);
    }

    auto h = get_active();
    if (h && !h.done()) h.resume();
  }
};

} // namespace sitar
