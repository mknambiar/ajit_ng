#pragma once

#include "coroutine_runtime.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int modular_thread_real_mode_enabled(void);
int modular_thread_ports_enabled(int core_id, int thread_id);

#ifdef __cplusplus
}

StepTaskT<void> modularCpuIcacheAccess(CoroutineOwner* owner,
                                       int core_id,
                                       int cpu_id,
                                       uint8_t context,
                                       uint8_t asi,
                                       uint32_t addr,
                                       uint8_t request_type,
                                       uint8_t byte_mask,
                                       uint8_t* mae,
                                       uint64_t* instr_pair,
                                       uint32_t* mmu_fsr);

StepTaskT<void> modularCpuDcacheAccess(CoroutineOwner* owner,
                                       int core_id,
                                       int cpu_id,
                                       uint8_t context,
                                       uint8_t asi,
                                       uint32_t addr,
                                       uint8_t request_type,
                                       uint8_t byte_mask,
                                       uint64_t write_data,
                                       uint8_t* mae,
                                       uint64_t* read_data);
#endif
