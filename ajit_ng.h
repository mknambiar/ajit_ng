struct dcache_out_struct {
	void *d_context_port, *d_asi_port, *d_addr_port;
	void *d_request_type_port, *d_byte_mask_port;
	void *d_write_data_port;
	uint8_t push_done, mae, is_trap1, is_dw;
	uint8_t is_load, is_store, is_atomic, is_swap, is_cswap, is_stbar;
}

typedef dcache_out_struct dcache_out;

struct icache_out_struct {
	void *i_context_port, *i_asi_port, *i_addr_port;
	void *i_request_type_port, *i_byte_mask_port;
    uint8_t push_done, mae;
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