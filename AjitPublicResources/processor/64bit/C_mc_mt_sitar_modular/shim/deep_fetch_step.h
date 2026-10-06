#pragma once

#include <cstdint>
#include "coroutine_runtime.h"

// Deep-yield memory bus access for fetch/MMU path.
// C++ overload used by coroutine-aware callers.
StepTaskT<int> sysMemBusRequest(CoroutineOwner* owner,
                                int core_id,
                                int thread_id,
                                uint8_t request_type,
                                uint8_t byte_mask,
                                uint32_t addr,
                                uint64_t data64,
                                uint64_t* rdata);

extern "C" void ajit_step_dump_deep_bus_summary(void);
