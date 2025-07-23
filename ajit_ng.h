struct dcache_out_struct;

//typedef uint32_t (*transaction_func_ptr)(struct dcache_out_struct*);

struct dcache_out_struct {
	void *d_context_port, *d_asi_port, *d_addr_port;
	void *d_request_type_port, *d_byte_mask_port;
	void *d_write_data_port;
	void *d_read_data_port;
	uint8_t push_done, mae, is_trap1, is_dw;
	uint8_t is_load, is_store, is_atomic, is_swap, is_cswap, is_stbar;
	uint8_t read_dword, even_odd;
	uint8_t cache_trasactions;
    //These two things need to be stored
    uint8_t addr_space, byte_mask, trap_read;
    uint32_t address, data;
	//uint32_t (*transaction_func_ptr)(int, int);
	//transaction_func_ptr func;
}

typedef dcache_out_struct dcache_out;

struct icache_out_struct {
	void *i_context_port, *i_asi_port, *i_addr_port;
	void *i_request_type_port, *i_byte_mask_port;
    uint8_t push_done, mae;
    uint8_t is_flush;
	uint8_t even_odd;
}

typedef icache_out_struct icache_out;

struct dcache_in_struct {
    void *d_mae_in_port;
    void *d_read_data_port;    
}

typedef dcache_in_struct dcache_in;

struct icache_in_struct {
    void *i_mae_in_port;
    void *i_inst_pair_port;
	void *i_mmu_fsr_port;
}

typedef icache_in_struct icache_in;