#ifndef __REGISTER_FILE_H__
#define __REGISTER_FILE_H__

#include <stdint.h>
#include "ImplementationDependent.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __Registerfile {
	uint32_t g[8];
	uint32_t r[16 * NWINDOWS];
	uint32_t f[32];
} RegisterFile;

void resetRegisterFile(RegisterFile* rf);
RegisterFile* makeRegisterFile();

void writeRegister(uint32_t pc, RegisterFile* rf, uint8_t addr, uint8_t cwp, uint32_t value);
void writeFRegister(RegisterFile* rf, uint8_t addr, uint32_t value);

uint32_t readRegister(RegisterFile* rf, uint8_t addr, uint8_t cwp);
uint32_t readFRegister(RegisterFile* rf, uint8_t addr);

#ifdef __cplusplus
}
#endif

#endif
