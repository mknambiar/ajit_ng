//MmuInterface.c
//AUTHOR: Neha Karanjkar



#ifdef SW
	#include <stdio.h>
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "RequestTypeValues.h"
#include "MmuS.h"

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


void mmu_cache_pull(mmu_in *mmu, uint8_t dcache, uint8_t *mmu_dword_command, uint8_t *request_type, uint32_t *addr, uint8_t *byte_mask, uint64_t *write_data)
{
    bool rc;
	bool sync = true;
	void *mmu_command_port, *request_type_port, *addr_port, *byte_mask_port;
	
	switch (dcache) {
		case 0:
			mmu_command_port = mmu->im_mmu_command_port;
			request_type_port = mmu->im_request_type_port;
			addr_port = mmu->im_addr_port;
			byte_mask_port = mmu->im_byte_mask_port; 
			break;
	
		case 1:
			mmu_command_port = mmu->dm_mmu_command_port;
			request_type_port = mmu->dm_request_type_port;
			addr_port = mmu->dm_addr_port;
			byte_mask_port = mmu->dm_byte_mask_port;

			rc = pulldword(mmu->dm_write_data_port, write_data, sync); 
			assert(rc == true);		    
			break;
            
        default:
            assert(0);
		
	}

    rc = pullchar(mmu_command_port, mmu_dword_command, sync); 
    assert(rc == true);
    rc = pullchar(request_type_port, request_type, sync); 
    assert(rc == true);
    rc = pullword(addr_port, addr, sync); 
    assert(rc == true);    
    rc = pullchar(byte_mask_port, byte_mask, sync); 
    assert(rc == true);    
}

void mmu_cache_pushback(mmu_in *mmu, uint8_t dcache)
{
    bool rc;
	bool sync = true;
	void *mae_port, *cacheable_port, *acc_port, *read_data_port, *read_line_port, *mmu_fsr_port, *synonym_invalidate_word_port;
	
	switch (dcache) {
		case 0:
			mae_port = mmu->im_mae_port;
			cacheable_port = mmu->im_cacheable_port;
			acc_port = mmu->im_acc_port;
			read_data_port = mmu->im_read_data_port;
			read_line_port = mmu->im_read_line_port;
			mmu_fsr_port = mmu->im_mmu_fsr_port;
			synonym_invalidate_word_port = mmu->im_synonym_invalidate_word_port;
			break;
	
		case 1:
			mae_port = mmu->dm_mae_port;
			cacheable_port = mmu->dm_cacheable_port;
			acc_port = mmu->dm_request_type_port;
			read_data_port = mmu->dm_read_data_port;
			read_line_port = mmu->dm_read_line_port;
			mmu_fsr_port = mmu->dm_mmu_fsr_port;
			synonym_invalidate_word_port = mmu->dm_synonym_invalidate_word_port;
			
			break;
		
        default:
            assert(0);
	}

    rc = pushchar(cacheable_port, mmu->cacheable, sync); 
    assert(rc == true); 
    rc = pushchar(acc_port, mmu->acc, sync); 
    assert(rc == true);
    if (mmu->read_line == 0) {
        rc = pushdword(read_data_port, mmu->read_data, sync); 
        assert(rc == true);
    } else {
        rc = pushline(read_line_port, (char *)&mmu->line_data, sync); 
        assert(rc == true);
    }
    rc = pushword(mmu_fsr_port, mmu->mmu_fsr, sync); 
    assert(rc == true);		    
    rc = pushword(synonym_invalidate_word_port, mmu->synonym_invalidate_word, sync); 
    assert(rc == true);	
    rc = pushchar(mae_port, mmu->mae, sync); 
    assert(rc == true);		    	
}

void sysMemBusRequest_push(mmu_in *mmu, uint8_t request_type, uint8_t byte_mask, uint32_t addr, uint64_t data64) {
    
    bool rc;
	bool sync = true;
    
    rc = pushchar(mmu->br_byte_mask_port, byte_mask, sync); 
    assert(rc == true); 
    rc = pushword(mmu->br_addr_port, addr, sync); 
    assert(rc == true); 
    rc = pushdword(mmu->br_data64_port, data64, sync); 
    assert(rc == true); 
    rc = pushchar(mmu->br_request_type_port, request_type, sync); 
    assert(rc == true); 
    mmu->bridge_push_count++;
    
}
