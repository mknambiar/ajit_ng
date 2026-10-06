#pragma once

#include <cstdint>

struct ModularThreadIcacheRequest {
  uint8_t kind = 0;
  uint8_t asi = 0;
  uint8_t context = 0;
  uint32_t addr = 0;
};

struct ModularThreadDcacheRequest {
  uint8_t kind = 0;
  uint8_t asi = 0;
  uint8_t context = 0;
  uint32_t addr = 0;
  uint64_t data = 0;
  uint8_t byte_mask = 0xff;
};

struct ModularMmuRequest {
  uint8_t source = 0;
  uint8_t thread_id = 0;
  uint8_t mmu_command = 0;
  uint8_t kind = 0;
  uint8_t asi = 0;
  uint8_t context = 0;
  uint32_t addr = 0;
  uint64_t data = 0;
  uint8_t byte_mask = 0xff;
};

struct ModularBridgeRequest {
  uint8_t source = 0;
  uint8_t thread_id = 0;
  uint8_t kind = 0;
  uint32_t addr = 0;
  uint64_t data = 0;
  uint8_t byte_mask = 0xff;
};

struct ModularResponse {
  uint8_t mae = 0;
  uint64_t data = 0;
};

// Internal cache/MMU metadata: physical address, terminal beat, permission
// certificate, and returned word index. Widths in SiTAR declarations are bytes.
constexpr uint64_t MOD_MMU_LAST = 1ull << 32;
constexpr uint64_t MOD_MMU_AUTHORIZED = 1ull << 33;
constexpr uint8_t MOD_MMU_WRITE_PHYSICAL = 0xf0;
bool modular_async_enabled();

struct ModularMmuResponse {
  uint8_t mae = 0;
  uint64_t data = 0;
  uint8_t cacheable = 0;
  uint8_t acc = 0;
  uint32_t mmu_fsr = 0;
  uint32_t synonym = 0;
  uint64_t meta = MOD_MMU_LAST;
};

bool modular_pull_thread_icache_request(void* valid,
                                        void* kind,
                                        void* asi,
                                        void* context,
                                        void* addr,
                                        ModularThreadIcacheRequest* req);
bool modular_pull_thread_icache_request_step(uint8_t* stage,
                                             void* valid,
                                             void* kind,
                                             void* asi,
                                             void* context,
                                             void* addr,
                                             ModularThreadIcacheRequest* req);
bool modular_push_thread_icache_request(void* valid,
                                        void* kind,
                                        void* asi,
                                        void* context,
                                        void* addr,
                                        const ModularThreadIcacheRequest& req);
bool modular_push_thread_icache_request_step(uint8_t* stage,
                                             void* valid,
                                             void* kind,
                                             void* asi,
                                             void* context,
                                             void* addr,
                                             const ModularThreadIcacheRequest& req);

bool modular_pull_thread_dcache_request(void* valid,
                                        void* kind,
                                        void* asi,
                                        void* context,
                                        void* addr,
                                        void* data,
                                        void* byte_mask,
                                        ModularThreadDcacheRequest* req);
bool modular_pull_thread_dcache_request_step(uint8_t* stage,
                                             void* valid,
                                             void* kind,
                                             void* asi,
                                             void* context,
                                             void* addr,
                                             void* data,
                                             void* byte_mask,
                                             ModularThreadDcacheRequest* req);
bool modular_push_thread_dcache_request(void* valid,
                                        void* kind,
                                        void* asi,
                                        void* context,
                                        void* addr,
                                        void* data,
                                        void* byte_mask,
                                        const ModularThreadDcacheRequest& req);
bool modular_push_thread_dcache_request_step(uint8_t* stage,
                                             void* valid,
                                             void* kind,
                                             void* asi,
                                             void* context,
                                             void* addr,
                                             void* data,
                                             void* byte_mask,
                                             const ModularThreadDcacheRequest& req);

bool modular_pull_mmu_request(void* valid,
                              void* source,
                              void* thread_id,
                              void* mmu_command,
                              void* kind,
                              void* asi,
                              void* context,
                              void* addr,
                              void* data,
                              void* byte_mask,
                              ModularMmuRequest* req);
bool modular_pull_mmu_request_step(uint8_t* stage,
                                   void* valid,
                                   void* source,
                                   void* thread_id,
                                   void* mmu_command,
                                   void* kind,
                                   void* asi,
                                   void* context,
                                   void* addr,
                                   void* data,
                                   void* byte_mask,
                                   ModularMmuRequest* req);
bool modular_push_mmu_request(void* valid,
                              void* source,
                              void* thread_id,
                              void* mmu_command,
                              void* kind,
                              void* asi,
                              void* context,
                              void* addr,
                              void* data,
                              void* byte_mask,
                              const ModularMmuRequest& req);
bool modular_push_mmu_request_step(uint8_t* stage,
                                   void* valid,
                                   void* source,
                                   void* thread_id,
                                   void* mmu_command,
                                   void* kind,
                                   void* asi,
                                   void* context,
                                   void* addr,
                                   void* data,
                                   void* byte_mask,
                                   const ModularMmuRequest& req);

bool modular_push_bridge_request(void* valid,
                                 void* source,
                                 void* thread_id,
                                 void* kind,
                                 void* addr,
                                 void* data,
                                 void* byte_mask,
                                 const ModularBridgeRequest& req);

bool modular_pull_response(void* valid, void* mae, void* data, ModularResponse* resp);
bool modular_pull_response_step(uint8_t* stage,
                                void* valid,
                                void* mae,
                                void* data,
                                ModularResponse* resp);
bool modular_push_response(void* valid, void* mae, void* data, const ModularResponse& resp);
bool modular_push_response_step(uint8_t* stage,
                                void* valid,
                                void* mae,
                                void* data,
                                const ModularResponse& resp);

bool modular_pull_mmu_response(void* valid,
                               void* mae,
                               void* data,
                               void* cacheable,
                               void* acc,
                               void* mmu_fsr,
                               void* synonym,
                               ModularMmuResponse* resp, void* meta = nullptr);
bool modular_pull_mmu_response_step(uint8_t* stage,
                                    void* valid,
                                    void* mae,
                                    void* data,
                                    void* cacheable,
                                    void* acc,
                                    void* mmu_fsr,
                                    void* synonym,
                                    ModularMmuResponse* resp, void* meta = nullptr);
bool modular_push_mmu_response(void* valid,
                               void* mae,
                               void* data,
                               void* cacheable,
                               void* acc,
                               void* mmu_fsr,
                               void* synonym,
                               const ModularMmuResponse& resp, void* meta = nullptr);
bool modular_push_mmu_response_step(uint8_t* stage,
                                    void* valid,
                                    void* mae,
                                    void* data,
                                    void* cacheable,
                                    void* acc,
                                    void* mmu_fsr,
                                    void* synonym,
                                    const ModularMmuResponse& resp, void* meta = nullptr);
