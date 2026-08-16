#ifndef __SITAR_CORE_H__
#define __SITAR_CORE_H__

#define MAX_NCORES 4
#define MAX_NTHREADS_PER_CORE 2

typedef struct _CoreState
{
	uint32_t core_id;
	uint32_t number_of_threads;
	ThreadState* threads[MAX_NTHREADS_PER_CORE];

	MmuState* mmu_state;
	WriteThroughAllocateCache* dcache;
	WriteThroughAllocateCache* icache;

	uint32_t page_access_status[32*1024];

	uint8_t has_multi_context_munit;
	uint8_t mmu_is_present;

} CoreState;

CoreState* makeCoreState(uint32_t core_id,
			uint32_t number_of_threads,
			uint32_t isa_mode,
			uint32_t bp_table_size,
			uint32_t icache_number_of_lines, uint32_t icache_associativity,
			uint32_t dcache_number_of_lines, uint32_t dcache_associativity,
			uint32_t tlb0_log_mem_size, uint32_t tlb0_log_set_size,
			uint32_t tlb1_log_mem_size, uint32_t tlb1_log_set_size,
			uint32_t tlb2_log_mem_size, uint32_t tlb2_log_set_size,
			uint32_t tlb3_log_mem_size, uint32_t tlb3_log_set_size,
			uint8_t report_traps, uint32_t init_pc);

void setPageBit(CoreState* s, uint32_t virtual_addr);

#endif
