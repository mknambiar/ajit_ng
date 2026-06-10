#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "AjitThread.h"
#include "Core.h"
#include "ThreadInterface.h"

void  setPageBit(CoreState* s, uint32_t virtual_addr)
{
	uint32_t page_id = (virtual_addr >> 12);
	uint32_t idx = page_id >> 5;

	uint32_t offset = (page_id & 0x1f);
	s->page_access_status[idx] = s->page_access_status[idx] | (1 << offset);
}

static void init_core(CoreState* s,
			uint32_t core_id,  
			uint32_t number_of_threads,
			uint32_t isa_mode,
			int bp_table_size,
			uint32_t icache_number_of_lines,  uint32_t icache_associativity,
			uint32_t dcache_number_of_lines,  uint32_t dcache_associativity,
			uint8_t report_traps, uint32_t init_pc)
{
	s->core_id = core_id;
	s->number_of_threads =  number_of_threads;

	s->has_multi_context_munit 	= hasMultiContextMunit(core_id); 
	s->mmu_is_present       = isMmuPresent(core_id);

	// make the memory subsystem.
	s->mmu_state      =  makeMmuState (core_id);
	s->icache         =  makeCache (core_id, 1, icache_number_of_lines, icache_associativity);
	s->dcache         =  makeCache (core_id, 0, dcache_number_of_lines, dcache_associativity);

	// make the threads.
	uint32_t I;
	for(I = 0; I < number_of_threads; I++)
	{
		s->threads[I] = makeThreadState(core_id, I, isa_mode, bp_table_size, report_traps, init_pc);
		s->threads[I]->mmu_state = s->mmu_state;
		s->threads[I]->dcache = s->dcache;
		s->threads[I]->icache = s->icache;
		s->threads[I]->parent_core_state = (CoreState*) s;
	
		char irl_buffer[256];
		sprintf(irl_buffer,"ENV_to_AJIT_irl_%d_%d", core_id, I);

		register_port(irl_buffer, 8, 1);
	}

	// page access bits.
	for(I=0; I < (32*1024); I++)
	{
		s->page_access_status[I] = 0;	
	}

	resetMmuState (s->mmu_state);
	flushCache (s->dcache);
	flushCache (s->icache);
}


CoreState*  makeCoreState(	uint32_t core_id,  
				uint32_t number_of_threads,
				uint32_t isa_mode, 
				uint32_t bp_table_size, 
				uint32_t icache_number_of_lines,  uint32_t icache_associativity,
				uint32_t dcache_number_of_lines,  uint32_t dcache_associativity,
				uint8_t report_traps,
				uint32_t init_pc)
{
	CoreState* s = (CoreState*) malloc(sizeof(CoreState));
	init_core (s, core_id, number_of_threads, isa_mode, bp_table_size, 
				icache_number_of_lines, icache_associativity,
				dcache_number_of_lines, dcache_associativity,
					report_traps, init_pc);
	return(s);
}
