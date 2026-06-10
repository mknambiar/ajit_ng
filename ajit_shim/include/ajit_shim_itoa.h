#ifndef AJIT_SHIM_ITOA_H
#define AJIT_SHIM_ITOA_H

#include <stdint.h>

// Converts an unsigned 32-bit integer to a null-terminated decimal string.
// The caller must provide a buffer of at least 11 bytes.
void ajit_shim_u32_to_dec(char *buf, uint32_t v);

#endif // AJIT_SHIM_ITOA_H
