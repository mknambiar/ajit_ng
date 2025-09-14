// DummyDcacheBehavior.c
//
//Dummy Dcache Behavior
//Simply forwards requests from CPU to MMU
//and forwards MMU side messages to CPU
// AUTHOR: Neha Karanjkar


#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include "ajit_ng.h"
#include "CacheInterfaceS.h"
#include "Ancillary.h"
#include "ASI_values.h"
#include "RequestTypeValues.h"
#include "MmuS.h"

extern pthread_mutex_t cache_mutex;

void cpuDcacheAccess_split_123 (
			/*uint8_t context,*/ // Context not needed now 
			WriteThroughAllocateCache* dcache,
            mmu_out *mmu,
            uint8_t asi, uint32_t addr, uint8_t request_type, uint8_t byte_mask,
            uint64_t write_data)
{

	uint8_t is_hit = 0;
    uint8_t first_is_not_cacheable;
    uint32_t line_addr;
    uint8_t context = 0;
    uint8_t lock_mask;
	uint8_t is_stbar, is_nop, is_flush,is_cached_mem_access,is_read, is_mmu_access, is_mmu_bypass;
            
	switch(mmu->mmu_push_count) {
		
		case 0:

			mmu->mae=0;
			mmu->read_data = 0;
			mmu->cacheable=0;
			mmu->acc = 0;
			
			mmu->mmu_state_updated = 0;
			mmu->do_mmu_read_dword = 0;
			mmu->do_mmu_write_dword = 0;
			mmu->do_mmu_fetch_line = 0;
				
			//clear the top-bit (thread-head bit) in asi.
			asi =asi&0x7F;

			// lock flag is the 6th bit.
			lock_mask    = (request_type & 0x40);	

			// the top bit is the new thread bit, which is ignored.
			// in new C ref model for 64-bit, bit 7 should be 0
			request_type = request_type & 0x3f;

		#ifdef DEBUG
			fprintf(stderr,"\n\nDCACHE :  read CPU request asi=0x%x, addr=0x%x, req_type=0x%x, byte_mask=0x%x, data=0x%lx\n",asi,addr,request_type,byte_mask,write_data);
		#endif

			//if this is a CCU request (made by the debugger),
			//translate this into a normal request.
			if(request_type==REQUEST_TYPE_CCU_CACHE_READ) 
			{
				request_type = REQUEST_TYPE_READ;
			}
			if(request_type==REQUEST_TYPE_CCU_CACHE_WRITE)
			{
				request_type = REQUEST_TYPE_WRITE;
			}

			decodeDcacheRequest(asi, request_type,
						&is_nop, &is_stbar, &is_flush, &is_read,
						&is_cached_mem_access, &is_mmu_access, &is_mmu_bypass);
			dcache->number_of_accesses++;
			if(is_flush)
			{
				// In principle, we could communicate the
				// flush to the MMU so that it can clear the RLUT.
				// If we do not do this, there will be some spurious
				// invalidates from the RLUT to the caches.. 
				dcache->number_of_flushes++;

				// flush and forget about it.
				flushCache(dcache);
				#ifdef DEBUG
				fprintf(stderr, "Info: DCACHE-%d flushed by flush-asi 0x%x\n", dcache->core_id, asi);
				#endif
			}
			else if (!is_stbar && !is_nop)
			{
				if(is_cached_mem_access)
				{

					uint8_t is_raw_hit = 0;
					lookupCache(dcache, context,
							addr, asi, &is_raw_hit,  &mmu->acc);
					int access_ok = accessPermissionsOk(request_type, asi, mmu->acc);

					is_hit = is_raw_hit && access_ok;

					if(is_hit)
					{
						dcache->number_of_hits++;
						if(is_read)
							dcache->number_of_read_hits++;
						else
							dcache->number_of_write_hits++;
					}
					else
					{
						dcache->number_of_misses++;
						if(is_read)
							dcache->number_of_read_misses++;
						else
							dcache->number_of_write_misses++;
					}
				}
				else if(ASI_MMU_ACCESS(asi))
				// induced flush.
				{
					dcache->number_of_flushes++;

					//
					// Note: In multi-context case,  do not flush on context-register write
					//      
					if(!dcache->multi_context || !isMmuContextPointerWrite(asi, addr))
					{
						flushCache(dcache);
					}
					#ifdef DEBUG
					fprintf(stderr,"Info: DCACHE-%d flushed by ASI-MMU-ACCESS, asi 0x%x\n", dcache->core_id, asi);
					#endif
				}
				else
				{
					dcache->number_of_bypasses++;
				}

				int is_cacheable_based_on_mmu_status = isCacheableRequestBasedOnMmuStatus (dcache);
				
				uint8_t do_mmu_read_dword  = 
						(is_read && (is_mmu_access  || is_mmu_bypass || 
									(lock_mask != 0) || 
									!is_cacheable_based_on_mmu_status));
				uint8_t do_mmu_write_dword = 
						!is_read && (is_cached_mem_access || is_mmu_access || is_mmu_bypass
									|| !is_cacheable_based_on_mmu_status);

				uint8_t do_mmu_fetch_line = (is_cached_mem_access && !(is_hit) && (lock_mask == 0) &&
									is_cacheable_based_on_mmu_status);

				if(is_read && (is_hit))
				{	
					mmu->read_data = getDwordFromCache	(dcache, context, addr);
                    
				}

				if(!is_read && (is_hit))
				{
					writeIntoLine (dcache, write_data, context, addr, byte_mask);
				}
				
				if ((is_hit) && (do_mmu_read_dword)) break;
				
				mmu->do_mmu_read_dword = do_mmu_read_dword;
				mmu->do_mmu_write_dword = do_mmu_write_dword;
				mmu->do_mmu_fetch_line = do_mmu_fetch_line;

				//forward the first request to MMU
				if(do_mmu_read_dword || do_mmu_write_dword)
				{
					uint8_t mmu_dword_command = 
						((request_type == REQUEST_TYPE_WRFSRFAR) ? MMU_WRITE_FSR :
							(do_mmu_read_dword ? MMU_READ_DWORD : 
								((is_hit) ? MMU_WRITE_DWORD_NO_RESPONSE : MMU_WRITE_DWORD)));

					mmu_push(mmu, mmu_dword_command, (request_type | lock_mask), asi,  addr,  byte_mask, write_data);
							
					break;
				}
				
			}
				
		case 1:
			first_is_not_cacheable = (mmu->do_mmu_read_dword || mmu->do_mmu_write_dword) && !(mmu->cacheable);
			// fetch the line from the mmu
			if(!(mmu->mae) && !first_is_not_cacheable && mmu->do_mmu_fetch_line)
			{

				mmu->mmu_push_count++; // We cwant to nudge it to case 2 for next call
				mmu_push(mmu, MMU_READ_LINE, request_type, asi, addr, byte_mask, write_data);
							
			}
				
		case 2: //count can be 2 or 3. Let it come here
		case 3: // One then this code makes sense
			line_addr = (addr & LINE_ADDR_MASK);
			//line_data[8];
            assert(mmu->read_line == 1);
            //memcpy(line_data, &mmu->read_data, sizeof(mmu->read_data));

			if(mmu->synonym_invalidate_word != 0)
			{
				uint32_t invalidate_va = (mmu->synonym_invalidate_word & 0x7fffffff);
				invalidateCacheLine(dcache, invalidate_va);
			}

			if(mmu->cacheable)
			{
				updateCacheLine (dcache, mmu->acc, context, line_addr, mmu->line_data);
				mmu->read_data = getDwordFromCache (dcache, context, addr);
			}
			else
			{
				mmu->read_data = mmu->line_data[cacheLineOffset(addr)];
			}
				
			//}
	} // switch
	
	if(asi == 0x4) // mmu control register update?
	{
		if((request_type == REQUEST_TYPE_WRITE) && (mmu->mmu_state_updated == 0)) {
			updateMmuState(dcache, byte_mask, addr, write_data);
			mmu->mmu_state_updated = 1;
		}
	}

#ifdef DEBUG
	fprintf(stderr,"\nDCACHE :  response to CPU mae=0x%x, data=0x%lx\n",mmu->mae, mmu->read_data);
#endif
	if(getCacheTraceFile() != NULL)
	{
		// I don't care about this for now
		dumpCpuDcacheAccessTrace(dcache, asi, addr, request_type, byte_mask, write_data,
							mmu->mae, mmu->read_data, is_hit);
	}
	//unlock_cache(dcache);
}


	
