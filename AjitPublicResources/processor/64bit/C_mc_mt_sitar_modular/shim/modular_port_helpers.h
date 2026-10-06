#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool mod_pull_bool(void* port, bool* value);
bool mod_push_bool(void* port, bool value);
bool mod_pull_u8(void* port, uint8_t* value);
bool mod_push_u8(void* port, uint8_t value);
bool mod_pull_u32(void* port, uint32_t* value);
bool mod_push_u32(void* port, uint32_t value);
bool mod_pull_u64(void* port, uint64_t* value);
bool mod_push_u64(void* port, uint64_t value);

#ifdef __cplusplus
}
#endif
