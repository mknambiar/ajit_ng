#pragma once

#ifdef __cplusplus
#include "../../shim/coroutine_runtime.h"
extern "C" {
#include "Mmu.h"
#include "CacheInterface.h"
}

StepTaskT<void> Mmu(CoroutineOwner* owner,
                    MmuState* ms,
                    int thread_id,
                    uint8_t mmu_command,
                    uint8_t request_type,
                    uint8_t asi,
                    uint32_t addr,
                    uint8_t byte_mask,
                    uint64_t write_data,
                    uint8_t* mae,
                    uint8_t* cacheable,
                    uint8_t* acc,
                    uint64_t* read_data,
                    uint32_t* mmu_fsr,
                    uint32_t* synonym_invalidate_word);

StepTaskT<void> cpuIcacheAccess(CoroutineOwner* owner,
                                int core_id,
                                int cpu_id,
                                uint8_t context,
                                MmuState* ms,
                                WriteThroughAllocateCache* icache,
                                uint8_t asi,
                                uint32_t addr,
                                uint8_t request_type,
                                uint8_t byte_mask,
                                uint8_t* mae,
                                uint64_t* instr_pair,
                                uint32_t* mmu_fsr);

StepTaskT<void> cpuDcacheAccess(CoroutineOwner* owner,
                                int core_id,
                                int cpu_id,
                                uint8_t context,
                                MmuState* ms,
                                WriteThroughAllocateCache* dcache,
                                uint8_t asi,
                                uint32_t addr,
                                uint8_t request_type,
                                uint8_t byte_mask,
                                uint64_t write_data,
                                uint8_t* mae,
                                uint64_t* read_data);
#endif
