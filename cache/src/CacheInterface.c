//CacheInterface.c
//
//CPU-side and memory side interface 
//of the Dcache and Icache
//AUTHOR: Neha Karanjkar



#ifdef SW
    #include <stdio.h>
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "ajit_ng.h"
#include "Ancillary.h"
#include "Ajit_Hardware_Configuration.h"
#include "RequestTypeValues.h"
#include "CacheInterfaceS.h"
#include "Ancillary.h"
#include "ASI_values.h"
//#include "Mmu.h"


extern int global_verbose_flag;

// Forward declare the wrapper functions
extern bool pullchar(void *obj, uint8_t *value, bool sync);
extern bool pushchar(void *obj, uint8_t value, bool sync);
extern bool pullword(void *obj, uint32_t *value, bool sync);
extern bool pullline(void *obj, char line_data[], bool sync);
extern bool pushword(void *obj,uint32_t value, bool sync);
extern bool pulldword(void *obj, uint64_t *value, bool sync);
extern bool pushdword(void *obj,uint64_t value, bool sync);
extern bool pullword_fromdword(void *obj, uint32_t *value, bool sync, uint8_t even_odd);
extern bool pushline(void *obj, char line_data[], bool sync);

FILE* cache_trace_file = NULL;
FILE* getCacheTraceFile(FILE* fp) {return(cache_trace_file);}

void writeIntoLine(WriteThroughAllocateCache* c, uint64_t write_data, uint8_t context, uint32_t va, uint8_t byte_mask)
{
	int I = cacheLineId (c, context, va);
	assert(I >= 0);

	int offset = cacheLineOffset (va);

	uint64_t vwal = c->cache_lines[I].cache_line[offset];
	vwal = insert_bytes_into_dword  (vwal,byte_mask, write_data);
	c->cache_lines[I].cache_line[offset] = vwal;
}

uint32_t cacheSetId    (WriteThroughAllocateCache* c, uint32_t va)
{
	uint32_t set_index =  
		(va >> LOG_BYTES_PER_CACHE_LINE) % c->number_of_sets;
	return(set_index);	
}

int cacheLineId (WriteThroughAllocateCache* c, uint8_t context, uint32_t va)
{
	int ret_val = -1;
	uint32_t set_id = cacheSetId(c, va);
	int I;
	for(I = 0; I < c->set_size; I++)
	{
		int J = (set_id * c->set_size) + I;
		if(c->cache_lines[J].valid &&
			(c->cache_lines[J].va_tag == vaTag(c, context, va)))
		{
			ret_val = J;
			break;
		}
	}
	return(ret_val);	
}

int cacheLineOffset (uint32_t va)
{
	int retval = ((va >> 3) & 0x7);
	return(retval);
}

uint32_t vaTag(WriteThroughAllocateCache* c, uint8_t context, uint32_t va)
{
	uint32_t ret_val  = 
		(va >> (LOG_BYTES_PER_CACHE_LINE + c->log_number_of_sets)) | (((uint32_t) context) << 24);
	return(ret_val);
}

uint64_t getDwordFromCache (WriteThroughAllocateCache* c, uint8_t context, uint32_t va)
{
	int index = cacheLineId(c, context, va);
	assert(index >= 0);

	int offset = cacheLineOffset(va);
	uint64_t retval = (c->cache_lines[index]).cache_line[offset];
	return(retval);
}

WriteThroughAllocateCache* makeCache (uint32_t core_id, int is_icache, int number_of_lines, int set_size)
{
	assert(number_of_lines <= MAX_NUMBER_OF_LINES);

	WriteThroughAllocateCache* nc = (WriteThroughAllocateCache*) 
						malloc (sizeof(WriteThroughAllocateCache));
	nc->core_id = core_id;
	nc->is_icache = is_icache;

	nc->number_of_lines = number_of_lines;
	nc->log_number_of_lines = calculate_log2(number_of_lines);

	nc->number_of_sets = (number_of_lines/set_size);
	nc->log_number_of_sets = calculate_log2(nc->number_of_sets);
	nc->set_size       = set_size;
	
	initCache(nc);	
	
	if(is_icache)
		sprintf(nc->from_rlut_pipe_name,"RLUT_to_ICACHE_%d", core_id);
	else
		sprintf(nc->from_rlut_pipe_name,"RLUT_to_DCACHE_%d", core_id);

	fprintf(stderr,"Info: made %s number_of_lines=%d, log_number_of_lines=%d\n",
				(is_icache ? "icache" : "dcache"),
				nc->number_of_lines, nc->log_number_of_lines);

	// ICACHE always thinks that mmu is enabled.
	
	if(is_icache)
	{
			nc->mmu_control_register = 1;
	}
	else
	{
			nc->mmu_control_register = 0;
	}

	nc->mmu_context_register = 0;

	return(nc);
}


void initCache (WriteThroughAllocateCache* c)
{
	c->number_of_accesses = 0;
	c->number_of_flushes  = 0;
	c->number_of_bypasses = 0;
	c->number_of_hits = 0;
	c->number_of_misses = 0;
	c->number_of_write_hits = 0;
	c->number_of_write_misses = 0;
	c->number_of_read_hits = 0;
	c->number_of_read_misses = 0;
	c->number_of_invalidates = 0;
	c->number_of_locked_accesses=0;

	int I; 
	for(I = 0; I < MAX_NUMBER_OF_LINES; I++)
		c->last_updated_offset_in_set[I] = -1;

	//c->lock_flag = 0;
	c->lock_core_id = 0;

	// multi-context support?
	c->multi_context = hasMultiContextMunit(c->core_id);

	flushCache(c);	
}

// invalidate the line at the specified index.
//  Note: line address is VA[31:6].. top bits are ignore, since we
//  only need to figure out the set_id.
//
void invalidateCacheLine (WriteThroughAllocateCache* c, uint32_t va_line_address)
{
	c->number_of_invalidate_requests++;

	int set_id = cacheSetId(c, (va_line_address << LOG_BYTES_PER_CACHE_LINE));
	if(set_id >= 0) 
	{
		int I;
		for(I = 0; I < c->set_size; I++)
		{	
			int J = (set_id * c->set_size) + I;
			if(c->cache_lines[J].valid)
			{
				c->cache_lines[J].valid = 0;
			}
		}
		c->number_of_invalidates++;

		if(global_verbose_flag)
			fprintf(stderr,"Info: %s core-id=%d, invalidated set %d (reduced va=0x%lx).\n", 
					(c->is_icache ? "icache" : "dcache"), 
					c->core_id, 
					set_id, 
					(va_line_address << 6)); 
	}
}


uint32_t allocateCacheLine (WriteThroughAllocateCache* c, uint8_t context, uint32_t va)
{
	uint32_t ret_val;
	int invalid_found = 0;

	uint32_t set_id = cacheSetId(c,va);
	assert (cacheLineId(c,context,va) < 0);

	int I;
	for(I = 0; I < c->set_size; I++)
	{
		int J = (set_id*c->set_size) + I;
		if(!(c->cache_lines[J].valid)	)
		{
			invalid_found = 1;	
			ret_val = J;

			c->last_updated_offset_in_set[set_id] = I;

			break;
		}
	}

	if(!invalid_found)
	{
		uint32_t index_within_set = (c->last_updated_offset_in_set[set_id] + 1) % c->set_size;

		ret_val      = (set_id * c->set_size) + index_within_set;
		c->cache_lines[ret_val].valid = 0;

		c->last_updated_offset_in_set[set_id] = index_within_set;
	}

	return(ret_val);
}



void lookupCache (WriteThroughAllocateCache* c,
		uint8_t context,
		uint32_t va, uint8_t asi, 
		uint8_t *hit,   uint8_t *acc)
{
	int I = cacheLineId(c, context, va);
	uint32_t va_tag = vaTag (c, context, va);

	*hit = 0;
	*acc = 0;

	if((I >= 0) && (c->cache_lines[I].valid))
	{
		if(c->cache_lines[I].va_tag == va_tag)
		{
			*hit = 1;
			*acc = c->cache_lines[I].acc;
		}
	}
	return;
}

void updateCacheLine (WriteThroughAllocateCache* c,
		uint8_t acc, 
		uint8_t context,
		uint32_t line_addr, 
		uint64_t* line_data)
{
	int I = cacheLineId (c, context, line_addr);
	if(I < 0)
	{
		I = allocateCacheLine(c,context, line_addr);
	}

	c->cache_lines[I].valid = 1;
	c->cache_lines[I].acc = acc;
	c->cache_lines[I].va_tag = vaTag(c, context, line_addr);

	int J;
	for (J=0; J < BYTES_PER_CACHE_LINE/8; J++)
		c->cache_lines[I].cache_line[J] = line_data[J];
}

void flushCache (WriteThroughAllocateCache* c)
{
	int I;
	for (I = 0; I < c->number_of_lines; I++)
	{
		c->cache_lines[I].valid = 0;
	}
}

void decodeDcacheRequest (uint8_t asi, 
		uint8_t request_type,
		uint8_t *is_nop,
		uint8_t *is_stbar,
		uint8_t *is_flush,
		uint8_t *is_read,
		uint8_t *is_cached_mem_access,
		uint8_t *is_mmu_access,
		uint8_t *is_mmu_bypass)
{
	*is_stbar      = 	(request_type==REQUEST_TYPE_STBAR);
	*is_nop        = 	(request_type==REQUEST_TYPE_NOP);
	*is_flush      =        ((asi >= 0x10) &&  (asi <= 0x14));
	*is_read       =  	(request_type == REQUEST_TYPE_READ);
	*is_cached_mem_access =     ((request_type == REQUEST_TYPE_READ) ||
			(request_type == REQUEST_TYPE_WRITE)) &&
		(asi >= 0x8) &&
		(asi <= 0xB);

	*is_mmu_access =    ASI_MMU_ACCESS(asi) || (request_type == REQUEST_TYPE_WRFSRFAR);
	*is_mmu_bypass =    ASI_MMU_PASS_THROUGH(asi);
}

void decodeIcacheRequest (uint8_t asi, 
		uint8_t request_type,
		uint8_t *is_nop,
		uint8_t *is_flush,
		uint8_t *is_ifetch)
{
	*is_nop        = 	(request_type==REQUEST_TYPE_NOP);
	*is_flush      =        (request_type == REQUEST_TYPE_WRITE) &&
		(((asi >= 0x10) &&  (asi <= 0x14)) ||
		 ((asi >= 0x18) && (asi <= 0x1C)));
	// ifetch
	*is_ifetch     =     (request_type == REQUEST_TYPE_IFETCH) && (asi >= 0x8) && (asi <= 0x9);
}

void printCacheStatistics (WriteThroughAllocateCache* c)
{
	fprintf(stderr,"Statistics for %s core-id=%d\n", (c->is_icache ? "ICACHE" : "DCACHE"), c->core_id);
	if(c->number_of_accesses > 0)
		fprintf(stderr,"    number-of-accesses=%ld\n", c->number_of_accesses);
	if(c->number_of_locked_accesses > 0)
		fprintf(stderr,"    number-of-locked-accesses=%d\n", c->number_of_locked_accesses);
	if(c->number_of_flushes > 0)
		fprintf(stderr,"    number-of-flushes=%ld\n", c->number_of_flushes);
	if(c->number_of_bypasses > 0)
		fprintf(stderr,"    number-of-bypasses=%ld\n", c->number_of_bypasses);
	if(c->number_of_hits > 0)
		fprintf(stderr,"    number-of-hits=%ld\n", c->number_of_hits);
	if(c->number_of_misses > 0)
		fprintf(stderr,"    number-of-misses=%ld\n", c->number_of_misses);
	if(c->number_of_write_hits > 0)
		fprintf(stderr,"    number-of-write_hits=%ld\n", c->number_of_write_hits);
	if(c->number_of_write_misses > 0)
		fprintf(stderr,"    number-of-write_misses=%ld\n", c->number_of_write_misses);
	if(c->number_of_read_hits > 0)
		fprintf(stderr,"    number-of-read_hits=%ld\n", c->number_of_read_hits);
	if(c->number_of_read_misses > 0)
		fprintf(stderr,"    number-of-read_misses=%ld\n", c->number_of_read_misses);
	if(c->number_of_invalidate_requests > 0)
		fprintf(stderr,"    number-of-invalidate-requests=%ld\n", c->number_of_invalidate_requests);
	if(c->number_of_invalidates > 0)
		fprintf(stderr,"    number-of-invalidates=%ld\n", c->number_of_invalidates);
}

int accessPermissionsOk (uint8_t request_type, uint8_t asi, uint8_t acc)
{
	int ret_val = 0;
	int is_supervisor = (asi == ASI_SUPERVISOR_INSTRUCTION) || (asi == ASI_SUPERVISOR_DATA);
	int ifetch = (asi == ASI_SUPERVISOR_INSTRUCTION) || (asi == ASI_USER_INSTRUCTION);
	int is_write = (request_type == REQUEST_TYPE_WRITE);
	switch(acc) {
		case 0: // read : read
			ret_val = (!ifetch && !is_write);
			break;
		case 1: // read/write : read/write
			ret_val = !ifetch;
			break;
		case 2: // read/exec: read/exec
			ret_val = !is_write;
			break;
		case 3: // read/write/exec : read/write/exec
			ret_val = 1;
			break;
		case 4: // exec : exec
			ret_val = ifetch;
			break;
		case 5: // read : read/write
			ret_val = (is_supervisor ? !ifetch : !(ifetch || is_write));
			break;
		case 6: // no-access: read/exec
			ret_val = is_supervisor && !is_write;
			break;
		case 7: 
			ret_val = is_supervisor;
			break;
	
	}
	return(ret_val);
}


void updateMmuState (WriteThroughAllocateCache* c, 
			uint8_t   byte_mask,
			uint32_t  waddr,
			uint64_t  write_dword)
{
	assert(!c->is_icache);

	uint32_t mask_value   = 0;	
	uint32_t upper_word = (write_dword >> 32) & 0xffffffff;

	int I;
	for(I = 7; I >= 4; I--)
	{
		if((byte_mask >> I) & 0x1) 
		{
			mask_value = (mask_value >> 8) | 0xff000000;
		}
	}
	uint32_t ctrl_reg_value = 0;

	switch ((waddr >> 8) & 0x7)
	{
		case 0:
			ctrl_reg_value = c->mmu_control_register;
			if(c->multi_context)
				c->mmu_control_register = (ctrl_reg_value & (~mask_value)) | (upper_word & mask_value);
			else 
			{
				// for all
					c->mmu_context_register = 0;
			}
			break;
		case 2:
			ctrl_reg_value = c->mmu_context_register;
			c->mmu_context_register = (ctrl_reg_value & (~mask_value)) | (upper_word & mask_value);
			break;
		default:
			break;
	}
#ifdef DEBUG
	fprintf(stderr,"Info:cache: updated mmu state : control=0x%x, context=0x%x\n", 
					c->mmu_control_register,
					c->mmu_context_register);
#endif
}


int isMmuContextPointerWrite (uint8_t asi, uint32_t va)
{
	int ret_val = 0;
	if(ASI_MMU_ACCESS(asi))
	{
		if(((va >> 8) & 0x7) == 2)
			ret_val = 1;
	}
	return(ret_val);
}

int isCacheableRequestBasedOnMmuStatus (WriteThroughAllocateCache* c)
{
	int ret_val = 0;
	if((c->mmu_control_register  & 0x1) || (c->mmu_control_register & (0x1 << 8)))
		ret_val = 1;
	return(ret_val);
}

void dumpCpuIcacheAccessTrace
		(WriteThroughAllocateCache* icache,
			uint8_t asi, uint32_t addr, uint8_t request_type, uint8_t byte_mask,
				uint8_t mae, uint64_t instr_pair, uint32_t mmu_fsr, uint8_t is_hit)
{

	fprintf(cache_trace_file,
			">>>>I (%d) 0x%x 0x%x 0x%x 0x%x :  0x%x 0x%llx 0x%x %s %s\n",
			icache->core_id, asi, addr, request_type, byte_mask, mae, instr_pair, mmu_fsr,
						((mmu_fsr != 0) ? "fsr!" : ""),
						(is_hit ? "hit": ""));
	fflush(cache_trace_file);
	
}

void dumpCpuDcacheAccessTrace
		     (WriteThroughAllocateCache* dcache,
			uint8_t asi, uint32_t addr, uint8_t request_type, uint8_t byte_mask,
				uint64_t write_data,
				uint8_t mae, uint64_t read_data, uint8_t is_hit)
{

	fprintf(cache_trace_file,
			">>>>D (%d) 0x%x 0x%x 0x%x 0x%x 0x%llx :  0x%x 0x%llx %s\n",
			dcache->core_id, asi, addr, request_type, byte_mask, write_data, mae, read_data,
					(is_hit ? "hit": ""));
	fflush(cache_trace_file);

}

void setCacheTraceFile(FILE* fp) 
{
	cache_trace_file = fp;
	fprintf(cache_trace_file,"// cache-trace file generated by AJIT C sim.\n");
	fprintf(cache_trace_file,"// format: for dcache\n");
	fprintf(cache_trace_file,"//    D (core)  asi addr req_type bytemask wdata : mae rdata\n");
	fprintf(cache_trace_file,"// format: for icache\n");
	fprintf(cache_trace_file,"//    I (core)  asi addr req_type bytemask : mae instr_pair mmu_fsr\n");
}

void cpuDcacheAccess_pull(void *d_addr_port, void *d_request_type_port, void *d_byte_mask_port, void *d_write_data_port,  
                uint32_t *addr, uint8_t *request_type, uint8_t *byte_mask, uint64_t *data64)
{
    bool rc;
	bool sync = true;
	
    rc = pullword(d_addr_port, addr, sync); 
    assert(rc == true);		
    rc = pullchar(d_request_type_port, request_type, sync); 
    assert(rc == true);		
    rc = pullchar(d_byte_mask_port, byte_mask, sync); 
    assert(rc == true);		    
    rc = pulldword(d_write_data_port, data64, sync); 
    assert(rc == true);		    
	
}

void cpuIcacheAccess_pull(void *d_addr_port, void *d_request_type_port, void *d_byte_mask_port,
                            uint32_t *addr, uint8_t *request_type, uint8_t *byte_mask)
{
    bool rc;
	bool sync = true;
	
    rc = pullword(d_addr_port, addr, sync); 
    assert(rc == true);		
    rc = pullchar(d_request_type_port, request_type, sync); 
    assert(rc == true);		
    rc = pullchar(d_byte_mask_port, byte_mask, sync); 
    assert(rc == true);		    
	
}

void mmu_pull(mmu_out *mmu)
{
    bool rc;
	bool sync = true;

    //rc = pullchar(mmu->mae_port, &mmu->mae, sync); 
    //assert(rc == true);
    rc = pullchar(mmu->cacheable_port, &mmu->cacheable, sync); 
    assert(rc == true); 
    rc = pullchar(mmu->acc_port, &mmu->acc, sync); 
    assert(rc == true);
    if (mmu->read_line == 0) {
        rc = pulldword(mmu->read_data_port, &mmu->read_data, sync); 
        assert(rc == true);
    } else {
        rc = pullline(mmu->read_line_port, (char *)&mmu->line_data, sync); 
        assert(rc == true);
    }
    rc = pullword(mmu->mmu_fsr_port, &mmu->mmu_fsr, sync); 
    assert(rc == true);		    
    rc = pullword(mmu->synonym_invalidate_word_port, &mmu->synonym_invalidate_word, sync); 
    assert(rc == true);		        
       
}

void mmu_push(mmu_out *mmu, uint8_t mmu_dword_command, uint8_t request_type, uint8_t asi, uint32_t addr, uint8_t byte_mask, uint64_t write_data)
{
    bool rc;
	bool sync = true;

    rc = pushchar(mmu->mmu_command_port, mmu_dword_command, sync); 
    assert(rc == true);
    rc = pushchar(mmu->request_type_port, request_type, sync); 
    assert(rc == true);
    rc = pushword(mmu->addr_port, addr, sync); 
    assert(rc == true);    
    rc = pushchar(mmu->byte_mask_port, byte_mask, sync); 
    assert(rc == true);		
    rc = pushdword(mmu->write_data_port, write_data, sync); 
    assert(rc == true);		    
    rc = pushchar(mmu->asi_port, asi, sync); 
    assert(rc == true);
	mmu->mmu_push_count++;
    mmu->read_line = (mmu_dword_command == MMU_READ_LINE)? 1:0;
}

void cpuDCacheAccess_pushback(void *d_mae_port, void *d_read_data_port,  uint8_t mae,  uint64_t data64)
{
    bool rc;
	bool sync = true;
	
    rc = pushdword(d_read_data_port, data64, sync); 
    assert(rc == true);		    
    rc = pushchar(d_mae_port, mae, sync); 
    assert(rc == true);		    
	
}

void cpuICacheAccess_pushback(void *i_mae_port, void *inst_pair_port,  void *mmu_fsr_port, 
                                uint8_t mae,  uint64_t data64, uint32_t mmu_fsr)
{
    bool rc;
	bool sync = true;
	
    rc = pushdword(inst_pair_port, data64, sync); 
    assert(rc == true);	
    rc = pushdword(mmu_fsr_port, mmu_fsr, sync); 
    assert(rc == true);	    
    rc = pushchar(i_mae_port, mae, sync); 
    assert(rc == true);		    
	
}
