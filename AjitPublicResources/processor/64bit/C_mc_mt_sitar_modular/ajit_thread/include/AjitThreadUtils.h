#ifndef __ajit_thread__utils_h____
#define __ajit_thread__utils_h____

#include <stdint.h>

int privilegesOk(uint8_t asi, uint8_t rwbar, uint8_t exec, uint8_t acc);

#endif
