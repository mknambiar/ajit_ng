#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * Packet contracts for the modular SiTAR AJIT simulator.
 *
 * These structs are the C-side representation of payloads that will be packed
 * onto SiTAR nets. Keep mutable state out of packet helpers; ownership belongs
 * to the receiving module.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ModularRequestKind {
  MOD_REQ_NOP = 0,
  MOD_REQ_IFETCH = 1,
  MOD_REQ_LOAD = 2,
  MOD_REQ_STORE = 3,
  MOD_REQ_STBAR = 4,
  MOD_REQ_FLUSH = 5,
  MOD_REQ_MMU_REG = 6,
  MOD_REQ_MMU_FLUSH_PROBE = 7
} ModularRequestKind;

typedef struct ModularCacheRequest {
  uint8_t valid;
  uint8_t core_id;
  uint8_t thread_id;
  uint8_t request_kind;
  uint8_t asi;
  uint8_t byte_mask;
  uint8_t context;
  uint8_t flags;
  uint32_t virtual_addr;
  uint64_t write_data;
} ModularCacheRequest;

typedef struct ModularCacheResponse {
  uint8_t valid;
  uint8_t mae;
  uint8_t cacheable;
  uint8_t access_permissions;
  uint32_t fsr;
  uint64_t read_data;
} ModularCacheResponse;

typedef struct ModularMmuRequest {
  uint8_t valid;
  uint8_t core_id;
  uint8_t thread_id;
  uint8_t source; /* 0 = I-cache, 1 = D-cache */
  uint8_t request_kind;
  uint8_t asi;
  uint8_t byte_mask;
  uint8_t context;
  uint32_t virtual_addr;
  uint64_t write_data;
} ModularMmuRequest;

typedef struct ModularMmuResponse {
  uint8_t valid;
  uint8_t mae;
  uint8_t cacheable;
  uint8_t access_permissions;
  uint32_t physical_addr;
  uint32_t fsr;
  uint32_t synonym_line;
  uint64_t read_data;
} ModularMmuResponse;

typedef struct ModularMemoryRequest {
  uint8_t valid;
  uint8_t core_id;
  uint8_t thread_id;
  uint8_t source;
  uint8_t write;
  uint8_t byte_mask;
  uint32_t physical_addr;
  uint64_t write_data;
} ModularMemoryRequest;

typedef struct ModularMemoryResponse {
  uint8_t valid;
  uint8_t mae;
  uint64_t read_data;
} ModularMemoryResponse;

#ifdef __cplusplus
}
#endif
