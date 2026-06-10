#pragma once
#include <stdint.h>

#ifdef __cplusplus
#include "coroutine_runtime.h"
extern "C" {
#endif

// One-time bridge initialization for core/thread state; call before simulation loop.
int ajit_thread_bridge_init(void);
// Emit compact end-of-run summary from bridged ajit threads.
void ajit_thread_bridge_dump_summary(void);
// Runtime topology helper for code paths that must map core/thread -> slot id.
int ajit_thread_bridge_get_threads_per_core(void);

#ifdef __cplusplus
}

// Coroutine-only bridge step used by the SITAR driver.
StepTaskT<int> ajit_thread_bridge_step_task(CoroutineOwner* owner, int id, uint64_t sim_time);
#endif
