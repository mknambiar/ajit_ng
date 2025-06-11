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
#include "ajit_ng.h"
#ifdef SW
#include <stdio.h>
#endif



//#define REQUEST_TYPE_IFETCH  0
//#define REQUEST_TYPE_READ   1
//#define REQUEST_TYPE_WRITE  2
//#define REQUEST_TYPE_STBAR  3
//#define REQUEST_TYPE_WRFSRFAR  4

void readInstructionPair(int core_id,
				int cpu_id,
				uint8_t context, 
				MmuState* ms,  WriteThroughAllocateCache* icache, 
				uint8_t asi, uint32_t addr, uint8_t* mae, uint64_t *ipair, 
				uint32_t* mmu_fsr)
{
	// set the top bit of the asi to 1 to indicate "thread head"
	cpuIcacheAccess(core_id, cpu_id, context, ms, icache, (asi | 0x80), 
					(addr & (~0x7)),  // double word address!
					REQUEST_TYPE_IFETCH,  0xff, mae, ipair, mmu_fsr);
}



//read an Instruction by accessing the instruction fetch interface
void readInstruction(int core_id, int cpu_id,
			uint8_t context, 
			MmuState* ms, WriteThroughAllocateCache* icache,
			uint8_t asi, uint32_t addr, uint8_t* mae, uint32_t *inst, uint32_t* mmu_fsr)
{

	uint64_t instr64;

	// set the top bit of the asi to 1 to indicate "thread head"
	cpuIcacheAccess(core_id, cpu_id, context, ms, icache, (asi | 0x80), 
					(addr & (~0x7)),  // double word address!
					REQUEST_TYPE_IFETCH,  0xff, mae, &instr64, mmu_fsr);
	*mae 	= (*mae & 0x1);

	//check if we need to return the even or odd word
	if(getBit32(addr,2)) 
		*inst = instr64;
	else 
		*inst = instr64>>32;

#ifdef DEBUG
	printf("\nCPU %d: IFETCH addr=0x%x, asi=0x%x, Instruction Fetched = 0x%x, MAE = 0x%x, MMU_FSR=0x%x",cpu_id,
			addr, asi, *inst, *mae, *mmu_fsr);
#endif
}

//Request ICACHE to flush line containing a given doubleword in sitar
void flushIcacheLine_sitar(uint8_t context, uint8_t asi, uint32_t addr, icache_out ic)
{

	uint64_t instr64;
	uint32_t mmu_fsr;

	// set the top bit of the asi to 1 to indicate "thread head"
    ic->push_done = 0;
	cpuIcacheAccess_push(context,  (asi | 0x80), addr, REQUEST_TYPE_WRITE,  0xff);
	*mae 	= (*mae & 0x1);
	
#ifdef DEBUG
	printf("\nCPU %d: ICACHE FLUSH addr=0x%x, asi=0x%x, MAE=0x%x, MMU_FSR=0x%x",
			cpu_id, addr, asi, *mae, mmu_fsr);
#endif
	return;
}


//Request ICACHE to flush line containing a given doubleword.
//returns mae=0 after flush is complete
void flushIcacheLine(int core_id, int cpu_id,
			uint8_t context,
			MmuState* ms, WriteThroughAllocateCache* icache, uint8_t asi, uint32_t addr, uint8_t* mae)
{

	uint64_t instr64;
	uint32_t mmu_fsr;

	// set the top bit of the asi to 1 to indicate "thread head"
	cpuIcacheAccess(core_id, cpu_id, context, ms, icache, (asi | 0x80), addr, REQUEST_TYPE_WRITE,  0xff, mae, &instr64, &mmu_fsr);
	*mae 	= (*mae & 0x1);
	
#ifdef DEBUG
	printf("\nCPU %d: ICACHE FLUSH addr=0x%x, asi=0x%x, MAE=0x%x, MMU_FSR=0x%x",
			cpu_id, addr, asi, *mae, mmu_fsr);
#endif
	return;
}

//Read a doubleword from Dcache - sitaar version
void readData64Base_sitar(uint8_t context,
			uint8_t debug_flag, uint8_t asi, uint8_t byte_mask, 
				uint32_t addr, dcache_out dc_out)
{

	uint8_t  request_type = (debug_flag ? REQUEST_TYPE_CCU_CACHE_READ : (REQUEST_TYPE_READ | IS_NEW_THREAD));

	if(asi==0)
	{
		printf("\nCPU %d: Trying to read data with asi =0 ! addr=0x%x",cpu_id, addr);
		//set breakpoint here
		exit(1);
	}

	//make sure the address is double-word-aligned
	//clear the last 3 bits.
	addr=addr&(0xFFFFFFF8);

	cpuDcacheAccess_push(context, asi, addr, request_type, byte_mask, 0x0, dc);
		
}


//Read a doubleword from Dcache
void readData64Base(int core_id, int cpu_id,
			uint8_t context,
			MmuState* ms, WriteThroughAllocateCache* dcache,
			uint8_t debug_flag, uint8_t asi, uint8_t byte_mask, 
				uint32_t addr, uint8_t* mae, uint64_t *data64)
{

	uint8_t  request_type = (debug_flag ? REQUEST_TYPE_CCU_CACHE_READ : (REQUEST_TYPE_READ | IS_NEW_THREAD));

	if(asi==0)
	{
		printf("\nCPU %d: Trying to read data with asi =0 ! addr=0x%x",cpu_id, addr);
		//set breakpoint here
		exit(1);
	}

	//make sure the address is double-word-aligned
	//clear the last 3 bits.
	addr=addr&(0xFFFFFFF8);

	cpuDcacheAccess (core_id, cpu_id, context, ms, dcache, asi, addr, request_type, byte_mask, 0x0, mae, data64);
	*mae 	= (*mae & 0x1);
}

//read a word from DCACHE.
void readDataBase(int core_id, int cpu_id, uint8_t context, MmuState* ms, WriteThroughAllocateCache* dcache,
			uint8_t debug_flag, uint8_t asi, uint8_t byte_mask,
			 uint32_t addr, uint8_t* mae, uint32_t *data)
{

	uint64_t data64;
	uint8_t byte_mask_64 = (addr & 0x4) ? byte_mask : (byte_mask << 4);

	readData64Base(core_id, cpu_id, context, ms, dcache, debug_flag, asi, byte_mask_64, addr, mae, &data64);

	if(getBit32(addr,2)) 
		*data = data64;
	else
		*data = (data64)>>32;
}

void readData(int core_id, int cpu_id, uint8_t context,  MmuState* ms, WriteThroughAllocateCache* dcache,
			 uint8_t asi,  uint8_t byte_mask,
		  	   uint32_t addr, uint8_t* mae, uint32_t *data)
{
	readDataBase(core_id, cpu_id, context, ms, dcache, 0,asi,byte_mask,addr,mae,data);
	#ifdef DEBUG
	printf("\nCPU %d: DCACHE READ WORD addr=0x%x, asi=0x%x, byte_mask=0x%x, WORD read = 0x%x, MAE = 0x%x",
			cpu_id, addr, asi,  byte_mask, *data, *mae);
	#endif

}

void readDataToDebug(int core_id, int cpu_id,
			uint8_t context,
			MmuState* ms, WriteThroughAllocateCache* dcache,
				uint8_t asi, uint8_t byte_mask, uint32_t addr, uint8_t* mae, uint32_t *data)
{
	readDataBase(core_id, cpu_id, context, ms,dcache,1,asi,byte_mask, addr,mae,data);
	#ifdef DEBUG
	printf("\nCPU %d: DCACHE CCU READ WORD addr=0x%x, asi=0x%x, byte-mask=0x%x, WORD read = 0x%x, MAE = 0x%x",
			cpu_id, addr, asi, byte_mask, *data, *mae);
	#endif
}

// sitar version of readData64
void readData64_sitar(uint8_t context,  uint8_t asi, uint8_t byte_mask, uint32_t addr, dcache_out dc_out)
{
	readData64Base_sitar(context, 0,asi,byte_mask,addr, dc_out);
	#ifdef DEBUG
	printf("\nCPU %d: DCACHE READ DWORD addr=0x%x, asi=0x%x, DWORD read = 0x%lx, MAE = 0x%x",
			cpu_id, addr, asi, *data, *mae);
	#endif
}

void readData64(int core_id, int cpu_id, uint8_t context,  MmuState* ms, WriteThroughAllocateCache* dcache,
			 uint8_t asi, uint8_t byte_mask, uint32_t addr, uint8_t* mae, uint64_t *data)
{
	readData64Base(core_id, cpu_id, context,  ms, dcache, 0,asi,byte_mask,addr,mae,data);
	#ifdef DEBUG
	printf("\nCPU %d: DCACHE READ DWORD addr=0x%x, asi=0x%x, DWORD read = 0x%lx, MAE = 0x%x",
			cpu_id, addr, asi, *data, *mae);
	#endif
}

void lockAndReadData(int core_id,
			int cpu_id,
			uint8_t context,
			MmuState* ms, WriteThroughAllocateCache* dcache,
			 uint8_t asi, uint8_t byte_mask, uint32_t addr, uint8_t* mae, uint32_t *data)
{
	uint64_t data64;

	uint8_t byte_mask_64 = (addr & 0x4) ? byte_mask : (byte_mask << 4);
	lockAndReadData64(core_id, cpu_id, context, ms, dcache, asi, byte_mask_64, addr, mae, &data64);
	if(getBit32(addr,2)) 
		*data = data64;
	else
		*data = (data64)>>32;
}

//sitar version
void lockAndReadData64_sitar(uint8_t context, uint8_t asi, uint8_t byte_mask, uint32_t addr, dcache_out dc_out)
{
	uint8_t  request_type = (REQUEST_TYPE_READ | IS_NEW_THREAD | SET_LOCK_FLAG);
	if(asi==0)
	{
		printf("\nCPU %d: Trying to read data with asi =0 ! addr=0x%x", cpu_id, addr);
		//set breakpoint here
		exit(1);
	}

	//make sure the address is double-word-aligned
	//clear the last 3 bits.
	addr=addr&(0xFFFFFFF8);

	cpuDcacheAccess_push(context, asi, addr, request_type, byte_mask, 0x0, dc_out);

}

void lockAndReadData64(int core_id,
		        int cpu_id,
			uint8_t context,
			MmuState* ms, WriteThroughAllocateCache* dcache,
			uint8_t asi, uint8_t byte_mask, uint32_t addr, uint8_t* mae, uint64_t *data)
{
	uint8_t  request_type = (REQUEST_TYPE_READ | IS_NEW_THREAD | SET_LOCK_FLAG);
	if(asi==0)
	{
		printf("\nCPU %d: Trying to read data with asi =0 ! addr=0x%x", cpu_id, addr);
		//set breakpoint here
		exit(1);
	}

	//make sure the address is double-word-aligned
	//clear the last 3 bits.
	addr=addr&(0xFFFFFFF8);

	cpuDcacheAccess (core_id, cpu_id, context, ms, dcache, asi, addr, request_type, byte_mask, 0x0, mae, data);
	*mae 	= (*mae & 0x1);
}

// Write MMU FSR, FAR through DCACHE for next gen Ajit
void updateMmuFsrFar_push(
			uint8_t context,
			uint32_t mmu_fsr, uint32_t mmu_far,
				(void *) context_port, (void *) asi_port, (void *) addr_port, (void *) request_type_port, 
				(void *) byte_mask_port, (void *) write_data_port)
{
	uint64_t data64 = 0;
	uint8_t mae;
    uint32_t addr = 0x0;
    uint8_t asi = 0x0;
	bool rc;
	bool sync = true;
    
	data64 = setSlice64(data64, 63,32, mmu_fsr);
	data64 = setSlice64(data64, 31,0,  mmu_far);
    
    // This should be a function by itself
    rc = pushchar(context_port, context, sync);
    assert(rc == true);	
    rc = pushchar(asi_port, asi, sync); 
    assert(rc == true);		
    rc = pushword(addr_port, addr, sync); 
    assert(rc == true);		
    rc = pushchar(request_type_port, REQUEST_TYPE_WRFSRFAR, sync); 
    assert(rc == true);		
    rc = pushchar(byte_mask_port, 0xff, sync); 
    assert(rc == true);		    
    rc = pushdword(write_data_port, data64, sync); 
    assert(rc == true);		    
        
}

// Write MMU FSR, FAR through DCACHE.
void updateMmuFsrFar(int core_id,
			int cpu_id,
			uint8_t context,
			MmuState* ms, WriteThroughAllocateCache* dcache,
					 uint32_t mmu_fsr, uint32_t mmu_far)
{
	uint64_t data64 = 0;
	uint8_t mae;
	data64 = setSlice64(data64, 63,32, mmu_fsr);
	data64 = setSlice64(data64, 31,0,  mmu_far);

	cpuDcacheAccess (core_id, cpu_id, context, ms, dcache, 0x0, 0x0, REQUEST_TYPE_WRFSRFAR, 0xff, data64, &mae, &data64);

	#ifdef DEBUG
	printf("\nCPU %d: MMU FSR FAR UPDATE. FSR=0x%x, FAR=0x%x to MMU through DCACHE", cpu_id, mmu_fsr, mmu_far);
	#endif

}

void cpuDcacheAccess_push(uint8_t context, uint8_t asi, uint32_t addr, uint8_t request_type,
				uint8_t byte_mask, uint64_t data64, dcache_out *dc)
{
    bool rc;
	bool sync = true;
	
	rc = pushchar(dc->d_context_port, context, sync);
    assert(rc == true);	
    rc = pushchar(dc->d_asi_port, asi, sync); 
    assert(rc == true);		
    rc = pushword(dc->d_addr_port, addr, sync); 
    assert(rc == true);		
    rc = pushchar(dc->d_request_type_port, request_type, sync); 
    assert(rc == true);		
    rc = pushchar(dc->d_byte_mask_port, byte_mask, sync); 
    assert(rc == true);		    
    rc = pushdword(dc->d_write_data_port, data64, sync); 
    assert(rc == true);		    
	dc->push_done = 1;
	
}

//(context,  (asi | 0x80), addr, REQUEST_TYPE_WRITE,  0xff);
				
void cpuIcacheAccess_push(uint8_t context, uint8_t asi, uint32_t addr, uint8_t request_type,
				uint8_t byte_mask, icache_out *ic)
{
        bool rc;
		bool sync = true;
	
///    
        
        rc = pushchar(ic->i_context_port, context, sync);
        assert(rc == true);	
        rc = pushchar(ic->i_asi_port, addr_space, sync); 
        assert(rc == true);		
        rc = pushword(ic->i_addr_port, addr & 0xfffffff8, sync); 
        assert(rc == true);		
        rc = pushchar(ic->i_request_type_port, request_type, sync); 
        assert(rc == true);		    
        rc = pushchar(ic->byte_mask_port, 0xff, sync); 
        assert(rc == true);		    
        ic->push_done = 1;
	
}
				
				
				
//write a double word (with a byte mask) to DCACHE for sitar
void writeData64Base_sitar(
				uint8_t context,
				uint8_t debug_flag, uint8_t asi, uint32_t addr, 
				uint8_t byte_mask, uint64_t data64, dcache_out *dc_out)
{

	uint8_t  request_type = (debug_flag ? REQUEST_TYPE_CCU_CACHE_WRITE : (REQUEST_TYPE_WRITE | IS_NEW_THREAD));
	if(asi==0)
	{
		printf("\nCPU %d: Trying to write data with asi =0 ! addr=0x%x",cpu_id, addr);
		//set breakpoint here
	}
	
	//make sure the address is double-word-aligned
	//clear the last 3 bits.
	addr=addr&(0xFFFFFFF8);

	cpuDcacheAccess_push(context, asi, addr, request_type, byte_mask, data64, dc_out);

	#ifdef DEBUG
	//printf("\nCPU %d: DCACHE WRITE addr=0x%x, asi=0x%x, request_type=0x%x, byte_mask=0x%x, data64 = 0x%lx, MAE = 0x%x",cpu_id, addr, asi, request_type, byte_mask, data64, *mae);
	#endif
}


//write a double word (with a byte mask) to DCACHE.
void writeData64Base(int core_id, int cpu_id,
				uint8_t context,
				MmuState* ms, WriteThroughAllocateCache* dcache,
				uint8_t debug_flag, uint8_t asi, uint32_t addr, 
				uint8_t byte_mask, uint64_t data64, uint8_t* mae)
{

	uint64_t read_data = 0;
	uint8_t  request_type = (debug_flag ? REQUEST_TYPE_CCU_CACHE_WRITE : (REQUEST_TYPE_WRITE | IS_NEW_THREAD));
	if(asi==0)
	{
		printf("\nCPU %d: Trying to write data with asi =0 ! addr=0x%x",cpu_id, addr);
		//set breakpoint here
	}
	
	//make sure the address is double-word-aligned
	//clear the last 3 bits.
	addr=addr&(0xFFFFFFF8);

	cpuDcacheAccess (core_id, cpu_id, context, ms, dcache, asi, addr, request_type, byte_mask, data64,
				mae, &read_data);
	*mae 	= (*mae & 0x1);

	#ifdef DEBUG
	printf("\nCPU %d: DCACHE WRITE addr=0x%x, asi=0x%x, request_type=0x%x, byte_mask=0x%x, data64 = 0x%lx, MAE = 0x%x",cpu_id, addr, asi, request_type, byte_mask, data64, *mae);
	#endif
}

void writeDataBase_sitar(uint8_t context, 
			 uint8_t debug_flag, uint8_t asi, uint32_t addr, 
			  uint8_t byte_mask, uint32_t data, dcache_out *dc_out)
{

	uint64_t data64 = data;
	if(getBit32(addr,2)==0)
	{
		byte_mask=byte_mask<<4;
		data64=data64<<32;
	}
	writeData64Base_sitar(context, debug_flag, asi, addr, byte_mask, data64, dc_out);

}

void writeDataBase(int core_id, int cpu_id, uint8_t context, MmuState* ms, WriteThroughAllocateCache* dcache,
			 uint8_t debug_flag, uint8_t asi, uint32_t addr, 
			  uint8_t byte_mask, uint32_t data, uint8_t* mae)
{

	uint64_t data64 = data;
	if(getBit32(addr,2)==0)
	{
		byte_mask=byte_mask<<4;
		data64=data64<<32;
	}
	writeData64Base(core_id, cpu_id, context, ms, dcache,  debug_flag, asi, addr, byte_mask, data64, mae);

}

void writeData64_sitar(uint8_t context,
			 uint8_t asi, uint32_t 
             addr, uint8_t byte_mask, uint64_t data, dcache_out *dc_out)
{
	writeData64Base_sitar(context, 0,asi,addr,byte_mask, data, dc_out);
}

void writeData64(int core_id, int cpu_id, uint8_t context, MmuState* ms, WriteThroughAllocateCache* dcache,
			 uint8_t asi, uint32_t 
             addr, uint8_t byte_mask, uint64_t data, uint8_t* mae)
{
	writeData64Base(core_id, cpu_id, context, ms, dcache, 0,asi,addr,byte_mask, data, mae);
}

void writeData_sitar(uint8_t context, uint8_t asi, uint32_t addr, uint8_t byte_mask, uint32_t data, dcache_out *dc_out)
{
	writeDataBase_sitar(context, 0, asi,addr,byte_mask, data, dc_out);
}

void writeData(int core_id, int cpu_id, uint8_t context, MmuState* ms, WriteThroughAllocateCache* dcache,
			 uint8_t asi, uint32_t addr, uint8_t byte_mask, uint32_t data, uint8_t* mae)
{
	writeDataBase(core_id, cpu_id, context, ms, dcache, 0, asi,addr,byte_mask, data, mae);
}

void writeDataFromDebug(int core_id, int cpu_id, uint8_t context, MmuState* ms, WriteThroughAllocateCache* dcache,
				 uint8_t asi, uint32_t addr, uint8_t byte_mask, uint32_t data, uint8_t* mae)
{
	writeDataBase(core_id, cpu_id, context, ms, dcache, 1,asi,addr,byte_mask, data, mae);
}

//send an STBAR request to the Dcache
void sendSTBAR(int core_id, int cpu_id, uint8_t context, MmuState* ms, WriteThroughAllocateCache* dcache)
{

	// ignored..
	uint8_t mae = 0;
	uint64_t read_data = 0;

	//send request
	cpuDcacheAccess (core_id, cpu_id, context,  ms, dcache, 0, 0, REQUEST_TYPE_STBAR | IS_NEW_THREAD,
				0,  0,  &mae, &read_data);
	#ifdef DEBUG
	printf("\nCPU %d: Sent STBAR request to the Dcache", cpu_id);
	#endif
}

//--------------------------------------------------
//CPU Interfaces implemented as signals :
//--------------------------------------------------
//functions to read interface pipes and set internal flags:
//void readBpIRL() { while(1) { bp_irl = read_uint8("ENV_to_AJIT_IRL"); }}
//void readBpReset() { while(1) { bp_reset = read_uint8("ENV_to_AJIT_reset_in"); }}
uint8_t getBpIRL(ThreadState* s) 
{
	//
	// ENV_to_AJIT_irl is read by interrupt controller
	// which will produce ENV_to_CPU_irl
	//
	uint8_t bp_irl = read_uint8(s->ilvl_pipe_name);
	return bp_irl; 
}


uint8_t getBpReset(ThreadState* s)
{
	uint8_t ret_val = read_uint8  (s->reset_pipe_name);
	return(ret_val);
}

uint8_t getBpFPUPresent(ThreadState* s) { return s->bp_fpu_present; } 
uint8_t getBpFPUException(ThreadState* s) { return s->bp_fpu_exception; }
uint8_t getBpFPUCc(ThreadState* s) { return s->bp_fpu_cc; }
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

uint8_t getPbError(ThreadState* s)
{
	return s->pb_error;
}

uint8_t getPbBlockLdstWord(ThreadState* s)
{
	return s->pb_block_ldst_word;
}

uint8_t getPbBlockLdstByte(ThreadState* s)
{
	return s->pb_block_ldst_byte;
}

//send a signature to logger pipe
void writeToLoggerPipe(ThreadState* s, uint32_t word)
{
	if(s->monitor_logger  != NULL)
		write_uint32(s->monitor_logger->logger_pipe_name, word);
	else
		assert(0);
}

