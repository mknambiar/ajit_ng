//Mmu.h
// AUTHOR: Neha Karanjkar

#ifndef _MMU_H_
#define _MMU_H_
#include <stdint.h>
#include "Ajit_Hardware_Configuration.h"
#include "ajit_ng.h"
#include "rlutS.h"
#ifdef USE_NEW_TLB
#include "tlbs.h"
#endif
#include "MmuCommands.h"

// Two threads can use an MMU for now.
// should be a power of two.
#define MMU_MAX_NUMBER_OF_THREADS  2

#define MMU_THREAD_MASK  (MMU_MAX_NUMBER_OF_THREADS-1)
//each entry in the TLB has the following fields:
typedef struct __TlbEntry
{
	uint32_t va_tag;	//virtual address tag
	uint32_t ctx_tag;	//context tag
	uint32_t pte;		//page table entry
	uint8_t	 pte_level;	//level at which this pte was found
				//in the page table hierarchy.
				//the level can be:
				//0 => PTE was found in context table
				//1 => PTE was found in Level-1 Table
				//2 => PTE was found in Level-2 Table
				//3 => PTE was found in Level-3 Table
} TlbEntry;


typedef struct _MmuState {

	uint32_t core_id;

	// Mmu Registers, maintained on a per-thread basis.
	uint32_t MmuControlRegister;
	uint32_t MmuContextTablePointerRegister;
	uint32_t MmuContextRegister;
	uint32_t MmuFaultStatusRegister;
	uint32_t MmuFaultAddressRegister;

	uint8_t  Mmu_FSR_FAULT_CLASS; //The type of the last FSR fault (NONE/IACCESS_FAULT/DACCESS_FAULT)

	//Variables for keep count of Mmu events
	uint32_t Num_Mmu_bypass_accesses;
	uint32_t Num_Mmu_probe_requests;
	uint32_t Num_Mmu_flush_requests;
	uint32_t Num_Mmu_register_reads;
	uint32_t Num_Mmu_register_writes;
	uint32_t Num_Mmu_translated_accesses;
	uint32_t Num_Mmu_TLB_hits;


#ifdef USE_NEW_TLB
	// new TLB implementation mirrors
	// hardware TLBs.
	setAssociativeMemory* tlb_0;
	setAssociativeMemory* tlb_1;
	setAssociativeMemory* tlb_2;
	setAssociativeMemory* tlb_3;
#else
	TlbEntry TLB_entries[TLB_SIZE];
#endif

	uint32_t counter;

	uint8_t mmu_is_present;
	uint8_t multi_context;

	uint8_t lock_flag;
	uint8_t lock_cpu_id;

} MmuState;


// Mmu Behavior
MmuState* makeMmuState (uint32_t core_id);
void resetMmuState (MmuState* ms);
void printMmuStatistics(MmuState* ms);
void Mmu_split_123(MmuState* ms,  
            mmu_in *mmu,
			uint8_t mmu_command,
			uint8_t request_type,
			uint8_t asi, uint32_t addr,	
			uint8_t byte_mask, uint64_t write_data);
void mmu_cache_pull(mmu_in *mmu, uint8_t dcache, uint8_t *mmu_dword_command, uint8_t *request_type, uint32_t *addr, uint8_t *byte_mask, uint64_t *write_data);
void mmu_cache_pushback(mmu_in *mmu, uint8_t dcache);
void sysMemBusRequest_push(mmu_in *mmu, uint8_t request_type, uint8_t byte_mask, uint32_t addr, uint64_t data64);
#endif
