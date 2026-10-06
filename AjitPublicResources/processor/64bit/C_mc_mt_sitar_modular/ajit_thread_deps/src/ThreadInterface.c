//CpuInterfaces.c

//cpu interface to other blocks.
//Implemented as pipes.
//
//AUTHORS: 	Sarath Mohan
//		Neha Karanjkar


#include <stdint.h>
#include <assert.h>
#include "Ancillary.h"
#include "Pipes.h"
#include "CacheInterface.h"
#include "monitorLogger.h"
#include "AjitThread.h"
#include "ThreadInterface.h"
#include "ajit_memory_shim.h"
#include "ajit_thread_bridge.h"
#ifdef SW
#include <stdio.h>
#endif

FILE* reg_write_ref;

//#define REQUEST_TYPE_IFETCH  0
//#define REQUEST_TYPE_READ   1
//#define REQUEST_TYPE_WRITE  2
//#define REQUEST_TYPE_STBAR  3
//#define REQUEST_TYPE_WRFSRFAR  4

//--------------------------------------------------
//CPU Interfaces implemented as signals :
//--------------------------------------------------
//functions to read interface pipes and set internal flags:
//void readBpIRL() { while(1) { bp_irl = read_uint8("ENV_to_AJIT_IRL"); }}
//void readBpReset() { while(1) { bp_reset = read_uint8("ENV_to_AJIT_reset_in"); }}
uint8_t getBpIRL(ThreadState* s) 
{
	#ifdef SW
	uint32_t threads_per_core = (uint32_t) ajit_thread_bridge_get_threads_per_core();
	if (threads_per_core == 0u) {
		threads_per_core = 2u;
	}
	int slot_id = (int) ((s->core_id * threads_per_core) + s->thread_id);
	ajit_shim_sample_irq(slot_id);
	return ajit_shim_get_irq(slot_id);
	#else
	//
	// ENV_to_AJIT_irl is read by interrupt controller
	// which will produce ENV_to_CPU_irl
	//
	uint8_t bp_irl = read_uint8(s->ilvl_pipe_name);
	return bp_irl;
	#endif
}


uint8_t getBpReset(ThreadState* s)
{
	uint8_t ret_val = read_uint8  (s->reset_pipe_name);
	return(ret_val);
}

uint8_t getBpFPUPresent(ThreadState* s) { return s->bp_fpu_present; } 
uint8_t getBpFPUException(ThreadState* s) { return s->bp_fpu_exception; }
uint8_t getBpCPPresent(ThreadState* s) { return s->bp_cp_present; }
uint8_t getBpCPException(ThreadState* s) { return s->bp_cp_exception; }
uint8_t getBpCPCc(ThreadState* s) { return s->bp_cp_cc; }




void setPbError(ThreadState* s, uint8_t val)
{
	s->pb_error = val;
	write_uint8(s->error_pipe_name, val);
}

void setPbBlockLdstWord(ThreadState* s, uint8_t val)
{
	s->pb_block_ldst_word = val;
	//write_uint8("pb_block_ldst_word", val); 
}

void setPbBlockLdstByte(ThreadState* s, uint8_t val)
{
	s->pb_block_ldst_byte = val;
	//write_uint8("pb_block_ldst_byte", val); // TODO
}

uint8_t getPbBlockLdstWord(ThreadState* s)
{
	return s->pb_block_ldst_word;
}

uint8_t getPbBlockLdstByte(ThreadState* s)
{
	return s->pb_block_ldst_byte;
}
