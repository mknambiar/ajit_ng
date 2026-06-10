//Mmu.c
//
//
//For documentation see MmuBehavior.txt
//and Appendix H in Sparc V8 Reference Manual
//AUTHOR: Neha Karanjkar

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

#include "Ajit_Hardware_Configuration.h"
#include "Ancillary.h"
#include "MmuInterface.h"
#include "ASI_values.h"
#include "RequestTypeValues.h"
#include "pthreadUtils.h"
#include "tlbs.h"
#include "TlbNew.h"
#include "CacheInterface.h"
#include "rlut.h"
#include "bridge.h"


#ifdef SW
#include<stdio.h>
#endif


//#define MMU_DEBUG

extern int   global_enable_statistic_collection;



//class of a fault
#define NOFAULT 0
#define IACCESS_FAULT 1
#define DACCESS_FAULT 2
#define __CACHEABLE__(ms,tid) ((ms->MmuControlRegister[tid] & 0x100) != 0)



//Helper functions for Mmu
uint8_t   isValidMmuRequest(uint8_t asi);
uint8_t	  isCacheRelatedRequest(uint8_t asi);
uint8_t   get_AT(uint8_t asi, uint8_t req_type);
uint8_t   getFaultType(uint8_t AT, uint32_t PTE);
void      updateFsrFar(MmuState* ms, int thread_id, uint32_t Fsr_val, uint32_t Far_val, uint8_t fault_class);
uint8_t   checkPageFaults(MmuState* ms,
		int thread_id,
		uint8_t pte_found, uint32_t pte, uint8_t pte_level, 
		uint8_t asi, uint32_t virt_addr, uint8_t request_type, 
		uint32_t* mmu_fsr_to_be_returned);
uint64_t  constructPhysicalAddr(uint32_t pte, uint8_t pte_level, uint32_t virt_addr);
uint8_t   isCacheable(uint32_t pte);







//check if a given memory access should be allowed
//to proceed as per the access permissions specified in ACC field in 
//the page table entry.
//
//If access should be allowed : 
//	return 1
//If access causes a page fault/protection fault/privilege fault:
//	update FSR/FAR registers to note the fault
//	return 0
//See ACC field in Appendix H, Sparc v8 manual.

uint8_t checkPageFaults(MmuState* ms,  int thread_id,
		uint8_t pte_found, uint32_t pte, uint8_t pte_level, 
		uint8_t asi, uint32_t virt_addr, uint8_t request_type, 
		uint32_t* fsr_to_cache)
{

	uint8_t AT=0;
	uint8_t fault_type=0;

	//Obtain Access Type from the asi and request_type:
	AT = get_AT(asi, request_type);

	//Get fault-type from the pte
	fault_type = getFaultType(AT, pte);

	//we have computed AT and FT.
	if(fault_type==0) return 1; //no fault has occured
	else
	{
		//A fault has occured. Calculate the new fsr value.
		uint32_t new_fsr_value = 0;

		//EBE
		new_fsr_value = setSlice32(new_fsr_value,17,10, 0x0);

		//L (pte level)
		new_fsr_value = setSlice32(new_fsr_value,9,8, pte_level);

		//AT (Access Type) 
		new_fsr_value = setSlice32(new_fsr_value,7,5, AT);

		//FT (Fault type) 
		new_fsr_value = setSlice32(new_fsr_value,4,2, fault_type);

		//FAV (Fault Addr Valid) = 1
		new_fsr_value = setSlice32(new_fsr_value,1,1, 0x1);

		//The OW (overflow) bit is determined inside
		//the updateFsrFar() routine.

		if(request_type != REQUEST_TYPE_IFETCH)
		{
			//if the request is NOT from ICACHE, 
			//then the FSR/FAR values are updated immediately.
			updateFsrFar(ms, thread_id, new_fsr_value, virt_addr, DACCESS_FAULT);

		}
		else
		{
			//If the request is from ICACHE,
			//FSR is not written, but is instead communicated
			// to the pipeline via the ICACHE.
			*fsr_to_cache = new_fsr_value;
		}
#ifdef MMU_DEBUG
		printf(" MMU fault occured : ACC = 0x%x, AT=0x%x, fault_type=0x%x, Fault address=0x%x", ACC, AT, fault_type, virt_addr);
#endif

		return 0;
	}
}







//return value of cacheable bit in a PTE
uint8_t isCacheable(uint32_t pte)
{
	return  getBit32(pte,7);
}

//construct physical address
//from pte, virtual address and pte_level
uint64_t constructPhysicalAddr(uint32_t pte, uint8_t pte_level, uint32_t virt_addr)
{
	//get page offset
	uint64_t page_offset;
	if	(pte_level==3) page_offset=getSlice32(virt_addr,11,0);
	else if (pte_level==2) page_offset=getSlice32(virt_addr,17,0);
	else if (pte_level==1) page_offset=getSlice32(virt_addr,23,0);
	else                   page_offset=getSlice32(virt_addr,31,0);

	//get page number from pte
	uint32_t PPN = getSlice32(pte,31,8);

	//construct physical address:
	uint64_t phy_addr=0;
	phy_addr = PPN;
	phy_addr = phy_addr<<12;
	phy_addr = phy_addr | page_offset;

	return phy_addr;
}



MmuState* makeMmuState (uint32_t core_id)
{
	MmuState* ms = (MmuState*) malloc (sizeof (MmuState));
	ms->core_id = core_id;
	pthread_mutex_init( & (ms->mmu_mutex), NULL);

	ms->counter = 0;

	ms->mmu_is_present = isMmuPresent(core_id);
	ms->multi_context =  hasMultiContextMunit(core_id);

	sprintf(ms->req_pipe, "AJIT_to_ENV_request_type_%d", core_id);
	sprintf(ms->addr_pipe, "AJIT_to_ENV_addr_%d", core_id);
	sprintf(ms->wdata_pipe, "AJIT_to_ENV_data_%d", core_id);
	sprintf(ms->byte_mask_pipe, "AJIT_to_ENV_byte_mask_%d", core_id);
	sprintf(ms->rdata_pipe, "ENV_to_AJIT_data_%d", core_id);

	// fully associative TLB with 2 entries.
	ms->tlb_0 = findOrAllocateSetAssociativeMemory(0,32,32,1,1);
	// fully associative with 8 entries.
	ms->tlb_1 = findOrAllocateSetAssociativeMemory(1,32,32,3,3);
	// fully associative with 16 entries.
	ms->tlb_2 = findOrAllocateSetAssociativeMemory(2,32,32,4,4);
	// 8-way set associative with 64 entries.
	ms->tlb_3 = findOrAllocateSetAssociativeMemory(3,32,32,6,3);

	resetMmuState (ms);

	return(ms);
}

//Initialize mmu state
void resetMmuState(MmuState* ms)
{
	int i;	
	for(i = 0; i < MMU_MAX_NUMBER_OF_THREADS; i++)
	{
		ms->MmuControlRegister[i]=0;
		ms->MmuContextTablePointerRegister[i]=0;
		ms->MmuContextRegister[i]=0;
		ms->MmuFaultStatusRegister[i]=0;
		ms->MmuFaultAddressRegister[i]=0;
		ms->Mmu_FSR_FAULT_CLASS[i]=NOFAULT;

		ms->Num_Mmu_bypass_accesses[i]=0;
		ms->Num_Mmu_probe_requests[i]=0;
		ms->Num_Mmu_flush_requests[i]=0;
		ms->Num_Mmu_register_reads[i]=0;
		ms->Num_Mmu_register_writes[i]=0;
		ms->Num_Mmu_translated_accesses[i]=0;
		ms->Num_Mmu_TLB_hits[i]=0;
	}

	initializeTlbNew(ms);
	ms->lock_flag = 0;
}


//return true if this is a cache-related request
//(such as cache flush/read cache data/tag)
//and should be ignored by the Mmu

uint8_t isCacheRelatedRequest(uint8_t asi)
{
	if(  asi==ASI_CACHE_TAG_I
			||asi==ASI_CACHE_DATA_I
			||asi==ASI_CACHE_TAG_I_D
			||asi==ASI_CACHE_DATA_I_D
			|| ASI_ICACHE_FLUSH(asi)
			|| ASI_DCACHE_FLUSH(asi)
	  )
	{
		return 1;
	} 
	else
		return 0;
}



//returns true, if the request coming from cpu-side 
//is a valid request
uint8_t isValidMmuRequest(uint8_t asi)
{
	if(  asi==ASI_BLOCK_COPY
			||asi==ASI_BLOCK_FILL
			||ASI_reserved(asi)
			||ASI_unassigned(asi)
	  )
	{
#ifdef SW
		fprintf(stderr,"\nMMU: ERROR. asi=0x%x not valid or not supported",asi);
#endif 
		return 0;
	}
	return 1;
}


//Compute access type (AT)
//from asi and request_type values.
//See Sparc manual appendix H
uint8_t get_AT(uint8_t asi, uint8_t req_type)
{
	uint8_t AT=0;

	//reads
	if(req_type==REQUEST_TYPE_READ)
	{
		if	(asi==ASI_USER_DATA) 		  AT = 0; //0       Load from User Data Space
		else if	(asi==ASI_SUPERVISOR_DATA)	  AT = 1; //1       Load from Supervisor Data Space
		else if	(asi==ASI_USER_INSTRUCTION)	  AT = 2; //2       Load/Execute from User Instruction Space
		else if	(asi==ASI_SUPERVISOR_INSTRUCTION) AT = 3; //3       Load/Execute from Supervisor Instruction Space
	}
	else if(req_type==REQUEST_TYPE_IFETCH)
	{
		if	(asi==ASI_USER_INSTRUCTION)	  AT = 2; //2       Load/Execute from User Instruction Space
		else if	(asi==ASI_SUPERVISOR_INSTRUCTION) AT = 3; //3       Load/Execute from Supervisor Instruction Space
	}
	else if(req_type==REQUEST_TYPE_WRITE)
	{
		if	(asi==ASI_USER_DATA)		 AT = 4; //4       Store to User Data Space
		else if	(asi==ASI_SUPERVISOR_DATA)	 AT = 5; //5       Store to Supervisor Data Space
		else if	(asi==ASI_USER_INSTRUCTION)   	 AT = 6; //6       Store to User Instruction Space
		else if	(asi==ASI_SUPERVISOR_INSTRUCTION)AT = 7; //7       Store to Supervisor Instruction Space
	}
	return AT;
}


//Compute fault-type for an access, by checking the
//page permissions (ACC field in the PTE).
//Returns 
//	- 0 if there is no fault 
//	- a non-zero fault-type value if the access causes a fault. 

uint8_t getFaultType(uint8_t AT, uint32_t PTE)
{
	uint32_t ET  = getSlice32(PTE,1,0);   	//type of PTE entry
	uint32_t ACC = getSlice32(PTE,4,2);   	//page access permissions
	uint8_t fault_type=0;   		//fault type

	//Compute fault-type	
	if(ET==0) fault_type=1; //PTE not found
	else if(ET==0x1 || ET==0x3) fault_type=4; //Invalid PTE
	else //check PTE permissions
	{
		switch(ACC)
		{
			case 0 : {
					 switch(AT)
					 {
						 case 2: case 3: case 4 : case 5 : case 6: case 7 : {fault_type = 2; break;} 
						 default : {fault_type=0; break;}
					 }
					 break;
				 }

			case 1 : {
					 switch(AT)
					 {
						 case 2: case 3: case 6: case 7 : {fault_type = 2; break;} 
						 default : {fault_type=0; break;}
					 }
					 break;
				 }

			case 2 : {
					 switch(AT)
					 {
						 case 4 : case 5 : case 6: case 7 : {fault_type = 2; break;} 
						 default : {fault_type=0; break;}
					 }
					 break;
				 }

			case 3 : {
					 {fault_type=0;}
					 break;
				 }


			case 4 : {
					 switch(AT)
					 {
						 case 0 : case 1: case 4 : case 5 : case 6: case 7 : {fault_type = 2; break;} 
						 default : {fault_type=0; break;}
					 }
					 break;
				 }

			case 5 : {
					 switch(AT)
					 {
						 case 2: case 3: case 4 : case 6: case 7 : {fault_type = 2; break;} 
						 default : {fault_type=0; break;}
					 }
					 break;
				 }


			case 6 : {
					 switch(AT)
					 {
						 case 5 : case 7 : {fault_type = 2; break;} 
						 case 0 : case 2 : case 4 : case 6 : {fault_type=3; break;} 								   
						 default : {fault_type=0; break;}
					 }
					 break;
				 }
			case 7 : {
					 switch(AT)
					 {
						 case 0 : case 2 : case 4 : case 6 : {fault_type=3; break;} 								   
						 default : {fault_type=0; break;}
					 }
					 break;
				 }
			default : fault_type = 0;
		}
	}
	return fault_type;
}


void updateFsrFar(MmuState* ms, int thread_id, uint32_t Fsr_val, uint32_t Far_val, uint8_t fault_class)
{
	//Overwrite (OW)=1 only if one IACCESS_FAULT tries to overwrite an existing IACCESS_FAULT
	//or a DACCESS_FAULT tries to overwrite another DACCESS_FAULT.
	if( ((ms->Mmu_FSR_FAULT_CLASS[thread_id] == IACCESS_FAULT) && (fault_class==IACCESS_FAULT)) 
			|| ((ms->Mmu_FSR_FAULT_CLASS[thread_id] == DACCESS_FAULT) && (fault_class==DACCESS_FAULT)) )
	{
		//set the OW bit
		Fsr_val = setBit32(Fsr_val,0,1);
	}
	else
	{	//clear the OW bit
		Fsr_val = setBit32(Fsr_val,0,0);
	}


	//The FSR/FAR registers can be updated if
	//the existing fault class was NOFAULT or IACCESS_FAULT.
	//A DACCESS_FAULT cannot be overwritten by an IACCESS_FAULT.
	if((ms->Mmu_FSR_FAULT_CLASS[thread_id] == NOFAULT) || (ms->Mmu_FSR_FAULT_CLASS[thread_id]==IACCESS_FAULT) || (fault_class==DACCESS_FAULT))
	{
		//Write the new fault status/addr
		ms->MmuFaultStatusRegister[thread_id] = Fsr_val;
		ms->MmuFaultAddressRegister[thread_id] = Far_val;
		ms->Mmu_FSR_FAULT_CLASS[thread_id] = fault_class;
		if(!ms->multi_context)
		{
			// When multiple MMU contexts are not supported, we
			// do not keep the threads separate
			int i;
			for(i = 0; i < MMU_MAX_NUMBER_OF_THREADS; i++)
			{
				ms->MmuFaultStatusRegister[i]  = Fsr_val;
				ms->MmuFaultAddressRegister[i] = Far_val;
				ms->Mmu_FSR_FAULT_CLASS[i]     = fault_class;
			}
		}
#ifdef MMU_DEBUG
		fprintf(stderr,"Info: MmuFrontReq: Updated FSR=0x%x, FAR=0x%x\n", Fsr_val, Far_val);
#endif
	}
}
