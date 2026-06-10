#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Phase-aware Ajit API wrapper (nonblocking begin/step).
void ajit_api_setDoubleWord_begin(int id, uint32_t addr, uint64_t data, uint8_t bm);
int  ajit_api_setDoubleWord(int id, uint64_t sim_time);

void ajit_api_getDoubleWord_begin(int id, uint32_t addr);
int  ajit_api_getDoubleWord(int id, uint64_t sim_time, uint64_t* out);

void ajit_api_accessMemU64_begin(int id, uint8_t rwbar, uint8_t bmask,
                                 uint32_t addr, uint64_t wdata);
int  ajit_api_accessMemU64(int id, uint64_t sim_time, uint64_t* rdata);

#ifdef __cplusplus
}
#endif
