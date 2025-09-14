#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "ajit_ng.h"
#include "CacheInterfaceS.h"
#include "MmuS.h"
#include "Ancillary.h"
#include "ASI_values.h"
#include "RequestTypeValues.h"

int icache_forward_to_mmu(int asi)
{
	if(ASI_MMU_ACCESS(asi)
	|| ASI_USER_OR_SUPV_ACCESS(asi)
	|| ASI_MMU_PASS_THROUGH(asi)
	 )
		return 1;
	else
		return 0;
}

void cpuIcacheAccess_split_12(
				WriteThroughAllocateCache* icache,
                mmu_out *mmu,
				uint8_t asi, uint32_t addr, uint8_t request_type, uint8_t byte_mask)
{	
	// service all the coherency related invalidates..

#ifdef DEBUG
	fprintf(stderr,"\n\nICACHE :  read CPU request asi=0x%x, addr=0x%x, req_type=0x%x, byte_mask=0x%x\n",asi,addr,request_type,byte_mask);
#endif
	
    uint64_t *instr_pair;
    uint32_t *mmu_fsr;
    uint8_t context = 0;
    uint8_t is_hit  = 0;
    
    instr_pair = &mmu->read_data;
    mmu_fsr = &mmu->mmu_fsr;
    
    if ((mmu->mmu_push_count)) {
        
        mmu->mae=0;
        *mmu_fsr = 0;
        *instr_pair  = 0;

        mmu->cacheable=0;
            
        //clear the top  bit (thread-head bit) in asi.
        asi=asi&0x7F;

        // not needed in new C ref model, bit 7 should be 0.
        request_type = request_type & 0x3f;

        uint8_t is_flush, is_nop, is_ifetch;        
        
        mmu->acc=0;

        decodeIcacheRequest (asi, request_type, &is_nop, &is_flush, &is_ifetch);
        icache->number_of_accesses++;
        if(is_flush)
        {
            icache->number_of_flushes++;
            flushCache(icache);
        }
        else if (is_ifetch)
        {
            uint8_t is_raw_hit = 0;
            lookupCache(icache, context,
                    addr, asi, &is_raw_hit,  &mmu->acc);
            int access_ok = accessPermissionsOk(request_type, asi, mmu->acc);
            is_hit = is_raw_hit && access_ok;

            if(is_hit)
            {	
                icache->number_of_hits++;
                *instr_pair = getDwordFromCache	(icache, context, addr);
                mmu->mae = (1 << 7) | (mmu->acc << 4);
            }
            else
            {
                icache->number_of_misses++;
                
                mmu->synonym_invalidate_word = 0;

                mmu_push(mmu, MMU_READ_LINE, REQUEST_TYPE_IFETCH, asi, addr, byte_mask, 0x0);

            }
        }
    } else {
        
        // mae fields
        //   7 6:4 3:2 1 0
        //   cacheable acc unused err exception.
        mmu->mae = (mmu->cacheable << 7) | (mmu->acc << 4) | (0x3 & mmu->mae);

        if(mmu->synonym_invalidate_word != 0)
        {
            uint32_t invalidate_va = 
                (mmu->synonym_invalidate_word & 0x7fffffff) << LOG_BYTES_PER_CACHE_LINE;
            invalidateCacheLine(icache, invalidate_va);
        }

        if(mmu->cacheable)
        {
            uint32_t line_addr = (addr & LINE_ADDR_MASK);
            
            updateCacheLine (icache, mmu->acc, context, line_addr, mmu->line_data);
            *instr_pair = getDwordFromCache (icache, context, addr);
        }
        else
        {
            *instr_pair = mmu->line_data[cacheLineOffset(addr)];
        }
    }


	if(getCacheTraceFile() != NULL) // Let it get dumped as many times. I don't care now
	{
		dumpCpuIcacheAccessTrace(icache, asi, addr, request_type, byte_mask, 
							mmu->mae, *instr_pair, *mmu_fsr, is_hit);
	}

#ifdef DEBUG
	fprintf(stderr,"\nICACHE :  response to CPU mae=0x%x, instr-pair=0x%lx, mmu-fsr=0x%x\n",
				mmu->mae,*instr_pair, *mmu_fsr);
#endif

}




