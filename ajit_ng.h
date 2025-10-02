//struct dcache_out_struct;

//typedef uint32_t (*transaction_func_ptr)(struct dcache_out_struct*);

#ifndef AJIT_NG

#define AJIT_NG

struct dcache_out_struct {
	void *d_asi_port, *d_addr_port;
	void *d_request_type_port, *d_byte_mask_port;
	void *d_write_data_port;
	void *d_read_data_port;
	uint8_t push_done, mae, is_trap1, is_dw;
	uint8_t is_load, is_store, is_atomic, is_swap, is_cswap, is_stbar;
	uint8_t read_dword, even_odd;
	uint8_t cache_transactions;
    //These two things need to be stored
    uint8_t addr_space, byte_mask, trap_read;
    uint32_t address, data;
	//uint32_t (*transaction_func_ptr)(int, int);
	//transaction_func_ptr func;
};

typedef struct dcache_out_struct dcache_out;

struct icache_out_struct {
	void *i_asi_port, *i_addr_port;
	void *i_request_type_port, *i_byte_mask_port;
	uint64_t inst_pair;
    uint8_t push_done, mae;
    uint8_t is_flush;
	uint8_t even_odd;
};

typedef struct icache_out_struct icache_out;

struct dcache_in_struct {
    void *d_mae_in_port;
    void *d_read_data_port;    
};

typedef struct dcache_in_struct dcache_in;

struct icache_in_struct {
    void *i_mae_in_port;
    void *i_inst_pair_port;
	void *i_mmu_fsr_port;
};

typedef struct icache_in_struct icache_in;

uint8_t arbitrate(uint8_t *mystatic_last_served, uint8_t *mystatic_fifo_number, uint8_t num_ports, void *obj[]);

struct mmu_out_struct {
    uint8_t mmu_push_count;
    void *mae_port;
    void *cacheable_port;
    void *acc_port;
    void *read_data_port;
    void *read_line_port;
    void *mmu_fsr_port;
    void *synonym_invalidate_word_port;
    void *mmu_command_port;
    void *request_type_port;
    void *asi_port;
    void *addr_port;	
    void *byte_mask_port; 
    void *write_data_port;
	
    uint8_t mae;
    uint8_t cacheable; 
    uint8_t acc;
    uint64_t read_data;
    uint64_t line_data[8]; //should ideally be defined as  BYTES_PER_CACHE_LINE/8
    uint32_t mmu_fsr;  //Only the ICache needs this
    uint8_t read_line;
    uint32_t synonym_invalidate_word;
	uint8_t do_mmu_read_dword;
	uint8_t do_mmu_write_dword;
	uint8_t do_mmu_fetch_line;
	uint8_t mmu_state_updated;
};

typedef struct mmu_out_struct mmu_out;

#define MMU_INIT 0
#define MMU_PASS_THRU_READ_LINE 1
#define MMU_PROBE 2
#define MMU_PHY_TRANSLATE 3 // Only in this case it moves to next 3 states if success.
#define MMU_READ_LINE_STATE 4
#define MMU_NOT_CACHEABLE_READ_WORD 5
#define MMU_NOT_READ_LINE 6
#define MMU_EXIT 7

//Also lets capture the mmu probe states
#define MMU_PROBE_IN 0
#define MMU_PROBE_OUT 1

//This happens in MMU_PROBE and MMU_PHY_TRANSLATE
#define WALK_PT_L0 0
#define WALK_PT_L1 1
#define WALK_PT_L2 2
#define WALK_PT_L3 3
#define WALK_PT_LRET 4

//This happens in read write page table entries
#define PTE_READ_WRITE_IN 0
#define PTE_READ_WRITE_OUT 1

//This happens in translate physical address
#define TRANS_PHYS_ENTRY 0
#define TRANS_PHYS_WALKING_1 1
#define TRANS_PHYS_WALKING_2 2
#define TRANS_PHYS_EXIT 3


struct mmu_in_struct {
    uint8_t bridge_push_count;
    void *dm_mae_port;
    void *dm_cacheable_port;
    void *dm_acc_port;
    void *dm_read_data_port;
    void *dm_read_line_port;
    void *dm_mmu_fsr_port;
    void *dm_synonym_invalidate_word_port;
    void *dm_mmu_command_port;
    void *dm_request_type_port;
    void *dm_asi_port;
    void *dm_addr_port;	
    void *dm_byte_mask_port; 
    void *dm_write_data_port;
	
    void *im_mae_port;
    void *im_cacheable_port;
    void *im_acc_port;
    void *im_read_data_port;
    void *im_read_line_port;
    void *im_mmu_fsr_port;
    void *im_synonym_invalidate_word_port;
    void *im_mmu_command_port;
    void *im_request_type_port;
    void *im_asi_port;
    void *im_addr_port;	
    void *im_byte_mask_port; 
    //bridge ports
    void *br_request_type_port;
    void *br_byte_mask_port;
    void *br_addr_port;
    void *br_data64_port;
    void *br_rdata_port;
    
	
	uint8_t mmu_exec_state, mmu_probe_state, mmu_walk_pt_state, mmu_read_pt_state, mmu_trans_phy_state;
    uint8_t mae;
    uint8_t cacheable; 
    uint8_t acc;
    uint64_t read_data;
    uint64_t line_data[8]; //should ideally be defined as  BYTES_PER_CACHE_LINE/8
    uint32_t mmu_fsr;  //Only the ICache needs this
    uint8_t read_line;
    uint32_t synonym_invalidate_word;
	uint8_t do_mmu_read_dword;
	uint8_t do_mmu_write_dword;
	uint8_t do_mmu_fetch_line;
	uint8_t mmu_state_updated;
    
    // some of the local variables
    uint64_t* ptr;
    uint8_t  TLB_hit, translation_success;
    uint32_t   va_tag;
    uint8_t read_line_count;
    uint64_t line_physical_addr;
    uint32_t updated_pte; //for translate_physical_address
};

typedef struct mmu_in_struct mmu_in;

#endif