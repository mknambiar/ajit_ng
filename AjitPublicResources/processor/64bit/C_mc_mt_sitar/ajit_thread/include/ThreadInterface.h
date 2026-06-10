#ifndef __SITAR_THREAD_INTERFACE_H__
#define __SITAR_THREAD_INTERFACE_H__

#include "RequestTypeValues.h"
#include "Mmu.h"
#include "CacheInterface.h"
#include "AjitThread.h"

uint8_t getBpIRL(ThreadState* s);
uint8_t getBpReset(ThreadState* s);
uint8_t getBpFPUPresent(ThreadState* s);
uint8_t getBpFPUException(ThreadState* s);
uint8_t getBpCPPresent(ThreadState* s);
uint8_t getBpCPException(ThreadState* s);
uint8_t getBpCPCc(ThreadState* s);
uint8_t getPbBlockLdstWord(ThreadState* s);
uint8_t getPbBlockLdstByte(ThreadState* s);

void setPbError(ThreadState* s, uint8_t val);
void setPbBlockLdstWord(ThreadState* s, uint8_t val);
void setPbBlockLdstByte(ThreadState* s, uint8_t val);

#endif
