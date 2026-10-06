#pragma once
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Must be called before any memory API usage.
void ajit_memory_shim_set_ports(int id,
                                void* act_port,
                                void* wr_port,
                                void* addr_port,
                                void* data_in_port,
                                void* data_out_port,
                                void* bm_port,
                                void* irq_port,
                                void* coh_fill_kind_port,
                                void* coh_fill_pa_line_port,
                                void* coh_fill_va_line_port,
                                void* coh_icache_inval_port,
                                void* coh_dcache_inval_port);

void ajit_memory_shim_set_irq_port(int id, void* irq_port);
void ajit_shim_sample_irq(int id);
uint8_t ajit_shim_get_irq(int id);
void ajit_shim_coherence_fill_begin(int id, uint8_t cache_kind, uint32_t pa_line_addr, uint32_t va_line_addr);
int  ajit_shim_coherence_fill(int id, uint64_t sim_time);
uint32_t ajit_shim_probe_coherence_invalidate(int core_id, int icache_flag);
int ajit_shim_console_try_read(uint8_t* out);
void ajit_shim_console_write(uint8_t value);
uint64_t ajit_shim_console_output_count(void);
uint64_t ajit_shim_console_prompt_count(void);

// Phase-aware, nonblocking shim API.
// Caller must call *_begin() once, then poll helper(sim_time, ...) until it returns true.
void ajit_shim_write_begin(int id, uint32_t addr, uint64_t data, uint8_t bm);
int  ajit_shim_write(int id, uint64_t sim_time);

void ajit_shim_read_begin(int id, uint32_t addr);
int  ajit_shim_read(int id, uint64_t sim_time, uint64_t* out);

// Shim-prefixed initialization helpers (no-op in shim)
void ajit_shim_setRandomizeFlag(int val);
int  ajit_shim_allocateMemory(unsigned int log_memory_size);
int  ajit_shim_initializeMemory(char* memoryMapFile);
void ajit_shim_setMemoryTraceFile(FILE* fp);

// Shim-prefixed memory API to avoid symbol clashes with real memory.c
uint8_t  ajit_shim_getByteInMemory(uint32_t address);
void     ajit_shim_setByteInMemory(uint32_t address, uint8_t byte);
uint32_t ajit_shim_getWordInMemory(uint32_t address);
void     ajit_shim_setWordInMemory(uint32_t address, uint32_t word, uint8_t byte_mask);
uint64_t ajit_shim_getDoubleWordInMemory(uint32_t address);
void     ajit_shim_vGetDoubleWordInMemory(uint32_t address, uint64_t* rd);
void     ajit_shim_setDoubleWordInMemory(uint32_t address, uint64_t double_word, uint8_t byte_mask);
void     ajit_shim_accessMemU64 (uint8_t rwbar, uint8_t bmask, uint32_t addr, uint64_t wdata, uint64_t* rdata);
void     ajit_shim_getQuadWordInMemory(uint32_t address, uint64_t* data_h, uint64_t* data_l);
void     ajit_shim_setQuadWordInMemory(uint32_t address, uint64_t data_h, uint64_t data_l);

#ifdef __cplusplus
}
#endif
