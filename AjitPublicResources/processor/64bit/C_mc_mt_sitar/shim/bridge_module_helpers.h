#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool bridge_pull_cpu_request_step(uint8_t* stage,
                                  bool* write_val,
                                  uint32_t* addr,
                                  uint64_t* data,
                                  uint8_t* bm,
                                  void* active_port,
                                  void* write_port,
                                  void* addr_port,
                                  void* data_port,
                                  void* bm_port);

bool bridge_issue_mem_read_step(uint8_t* stage,
                                void* mem_act_port,
                                void* mem_wr_port,
                                void* mem_addr_port,
                                uint32_t addr);

bool bridge_issue_mem_write_step(uint8_t* stage,
                                 void* mem_act_port,
                                 void* mem_wr_port,
                                 void* mem_addr_port,
                                 void* mem_data_port,
                                 void* mem_bm_port,
                                 uint32_t addr,
                                 uint64_t data,
                                 uint8_t bm);

bool bridge_collect_mem_response(void* mem_data_port, uint64_t* value);
bool bridge_send_cpu_response_step(uint8_t* stage, void* cpu_data_port, uint64_t value);

#define BRIDGE_TARGET_MEMORY     0
#define BRIDGE_TARGET_SCRATCHPAD 1
#define BRIDGE_TARGET_TIMER      2
#define BRIDGE_TARGET_IRC        3
#define BRIDGE_TARGET_SERIAL_TX  4
#define BRIDGE_TARGET_SERIAL_RX  5

uint8_t bridge_decode_target(uint32_t addr);
bool bridge_issue_peripheral_access_step(uint8_t* stage,
                                         void* req_port,
                                         bool rwbar,
                                         uint8_t byte_mask,
                                         uint32_t addr,
                                         uint32_t data);
bool bridge_collect_peripheral_response(void* resp_port, uint32_t* value);
bool bridge_issue_target_access_step(uint8_t* stage,
                                     uint8_t target_kind,
                                     void* sp_req_port,
                                     void* timer_req_port,
                                     void* irc_req_port,
                                     void* serial_tx_req_port,
                                     void* serial_rx_req_port,
                                     bool rwbar,
                                     uint8_t byte_mask,
                                     uint32_t addr,
                                     uint32_t data);
bool bridge_collect_target_response(uint8_t target_kind,
                                    void* sp_resp_port,
                                    void* timer_resp_port,
                                    void* irc_resp_port,
                                    void* serial_tx_resp_port,
                                    void* serial_rx_resp_port,
                                    uint32_t* value);
uint32_t bridge_insert_using_byte_mask32(uint32_t old_val, uint32_t new_val, uint8_t byte_mask);

bool memory_pull_request_step(uint8_t* stage,
                              bool* write_val,
                              uint32_t* addr,
                              uint64_t* data,
                              uint8_t* bm,
                              void* active_port,
                              void* write_port,
                              void* addr_port,
                              void* data_port,
                              void* bm_port);

bool memory_send_response_step(uint8_t* stage, void* data_out_port, uint64_t value);

bool l2_cache_access_step(uint8_t* stage,
                          uint8_t this_phase,
                          bool write_val,
                          uint8_t byte_mask,
                          uint32_t addr,
                          uint64_t data,
                          uint64_t* response,
                          void* mem_act_port,
                          void* mem_wr_port,
                          void* mem_addr_port,
                          void* mem_data_port,
                          void* mem_bm_port,
                          void* mem_resp_port);

bool peripheral_pull_request_step(uint8_t* stage,
                                  bool* rwbar,
                                  uint8_t* byte_mask,
                                  uint32_t* addr,
                                  uint32_t* data,
                                  void* req_port);

bool peripheral_send_response_step(uint8_t* stage, void* resp_port, uint32_t value);

bool signal_pull_u8(void* port, uint8_t* value);
bool signal_push_u8(void* port, uint8_t value);
bool signal_pull_u32(void* port, uint32_t* value);
bool signal_push_u32(void* port, uint32_t value);

bool bridge_poll_coherence_fill_step(uint8_t* stage,
                                     uint8_t core_id,
                                     uint8_t* cache_kind,
                                     uint32_t* pa_line_addr,
                                     uint32_t* va_line_addr,
                                     void* cache_kind_port,
                                     void* pa_line_addr_port,
                                     void* va_line_addr_port);
void bridge_snoop_filter_note_memory_read(uint8_t core_id, uint32_t pa_line_addr);
bool bridge_emit_coherence_invalidate_step(uint8_t* core_index,
                                           uint8_t* cache_kind,
                                           uint8_t src_core_id,
                                           uint32_t pa_line_addr,
                                           uint8_t num_cores,
                                           void* icache_inval_port,
                                           void* dcache_inval_port);

#ifdef __cplusplus
}
#endif
