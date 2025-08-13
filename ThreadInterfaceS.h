//-------------------------------------------------------------------------
// Interface signals (Appendix C Page 153)
//-------------------------------------------------------------------------

#ifndef __INTERFACES_H__
#define __INTERFACES_H__

#include "RequestTypeValues.h"
//#include "CacheInterface.h"
#include "AjitThreadS.h"


//Request ICACHE to flush line containing a given doubleword.
//returns mae=0 after flush is complete
void flushIcacheLine_sitar(uint8_t asi, uint32_t addr, icache_out *ic);

//flush the entire I cache
void flushIcache_sitar(icache_out *ic);

void cpuDcacheAccess_push(uint8_t asi, uint32_t addr, uint8_t request_type,
				uint8_t byte_mask, uint64_t data64, dcache_out *dc);
                
void cpuIcacheAccess_push(uint8_t asi, uint32_t addr, uint8_t request_type,
				uint8_t byte_mask, icache_out *ic);

//read a word from DCACHE.
//  This routine fetches an aligned  doubleword, 
//  and returns one word from the double-word
//  to the cpu.
void readData_sitar(uint8_t asi,  uint8_t byte_mask, uint32_t addr, dcache_out *dc_out);

// starts the read and locks the bus.
void lockAndReadData_sitar(uint8_t asi, uint8_t byte_mask, uint32_t addr, dcache_out *dc_out);

void readData64_sitar(uint8_t asi, uint8_t byte_mask, uint32_t addr, dcache_out *dc_out);

void lockAndReadData64_sitar(uint8_t asi, uint8_t byte_mask, uint32_t addr, dcache_out *dc_out);


//write a word (with a byte mask) to DCACHE.
void writeData_sitar(uint8_t asi, uint32_t addr, uint8_t byte_mask, uint32_t data, dcache_out *dc_out);

//write a double word (with a byte mask) to DCACHE.
void writeData64_sitar(uint8_t asi, uint32_t addr, uint8_t byte_mask, uint64_t data, dcache_out *dc_out);

// same as above but from Debug.  Ignores trap information in caches.

//send an STBAR request to the Dcache
void sendSTBAR_sitar(dcache_out *dc_out);

//functions to read interface pipes and set internal flags:
void readBpIRL(ThreadState* s); 
void readBpReset(ThreadState* s) ;
uint8_t getBpIRL(ThreadState* s);

//functions used by cpu to read flags
uint8_t getBpReset(ThreadState* s);
uint8_t getBpFPUPresent(ThreadState* s)  ;
uint8_t getBpFPUException(ThreadState* s) ;
uint8_t getBpFPUCc(ThreadState* s);
uint8_t getBpCPPresent(ThreadState* s) ;
uint8_t getBpCPException(ThreadState* s) ;
uint8_t getBpCPCc(ThreadState* s);
uint8_t getPbError(ThreadState* s);
uint8_t getPbBlockLdstWord(ThreadState* s);
uint8_t getPbBlockLdstByte(ThreadState* s);

//functions used by cpu to write to flags
void setPbError(ThreadState* s, uint8_t val);
void setPbBlockLdstWord(ThreadState* s, uint8_t val);
void setPbBlockLdstByte(ThreadState* s, uint8_t val);

//send a signature to logger pipe
void writeToLoggerPipe(ThreadState* s, uint32_t word);

ThreadState* makeThreadState(uint32_t core_id,
					uint32_t thread_id, int isa_mode, int bp_table_size,
					int report_traps, uint32_t init_pc);


// Register write trace logging
#include <stdio.h>
//FILE* reg_write_ref;    // File pointer of logger-trace
#endif
