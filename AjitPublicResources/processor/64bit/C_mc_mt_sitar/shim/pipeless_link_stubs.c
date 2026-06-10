#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "AjitThread.h"

// Pipe-free link stubs to keep real ajit_thread_step buildable in Sitar mode.
// Real implementations can override these weak symbols later.

uint8_t __attribute__((weak)) read_uint8(const char* id)
{
	(void) id;
	return 0;
}

uint32_t __attribute__((weak)) read_uint32(const char* id)
{
	(void) id;
	return 0;
}

uint64_t __attribute__((weak)) read_uint64(const char* id)
{
	(void) id;
	return 0;
}

void __attribute__((weak)) write_uint8(const char* id, uint8_t data)
{
	(void) id;
	(void) data;
}

void __attribute__((weak)) write_uint32(const char* id, uint32_t data)
{
	(void) id;
	(void) data;
}

void __attribute__((weak)) write_uint64(const char* id, uint64_t data)
{
	(void) id;
	(void) data;
}

char* __attribute__((weak)) getCacheInvalPipeName(int cpu_id, int icache_flag)
{
	static char pipe_name[64];
	(void) cpu_id;
	(void) icache_flag;
	strcpy(pipe_name, "SITAR_PIPELESS_CACHE_INVAL");
	return pipe_name;
}

uint32_t __attribute__((weak)) lookupAndUpdateRlutInManager(int cpu_id,
							    uint32_t pa_line_addr,
							    uint32_t va_line_addr,
							    int icache_flag)
{
	(void) cpu_id;
	(void) pa_line_addr;
	(void) va_line_addr;
	(void) icache_flag;
	return 0;
}

void __attribute__((weak)) flushRlutInManager(int cpu_id, int icache_flag)
{
	(void) cpu_id;
	(void) icache_flag;
}

uint32_t __attribute__((weak)) assembleRegisterWriteSignature(uint8_t psr_updated,
							      uint32_t psr_value,
							      uint8_t wim_updated,
							      uint32_t wim_value,
							      uint8_t tbr_updated,
							      uint32_t tbr_value,
							      uint8_t y_updated,
							      uint32_t y_value,
							      uint8_t gpr_write,
							      uint8_t double_word_write,
							      uint8_t reg_id,
							      uint32_t reg_val_high,
							      uint32_t reg_val_low)
{
	(void) psr_updated;
	(void) psr_value;
	(void) wim_updated;
	(void) wim_value;
	(void) tbr_updated;
	(void) tbr_value;
	(void) y_updated;
	(void) y_value;
	(void) gpr_write;
	(void) double_word_write;
	(void) reg_id;
	return (reg_val_high ^ reg_val_low);
}

uint32_t __attribute__((weak)) assembleFpRegisterWriteSignature(uint8_t write_fpreg_from_fpunit,
								uint8_t write_fpreg_from_ccu,
								uint8_t write_fsr_from_fpunit,
								uint8_t write_fsr_from_ccu,
								uint8_t write_full_fsr,
								uint8_t write_ftt,
								uint8_t write_fcc,
								uint8_t write_double,
								uint8_t write_reg_id,
								uint32_t fsr_val,
								uint32_t reg_val_h,
								uint32_t reg_vl_l)
{
	(void) write_fpreg_from_fpunit;
	(void) write_fpreg_from_ccu;
	(void) write_fsr_from_fpunit;
	(void) write_fsr_from_ccu;
	(void) write_full_fsr;
	(void) write_ftt;
	(void) write_fcc;
	(void) write_double;
	(void) write_reg_id;
	(void) fsr_val;
	return (reg_val_h ^ reg_vl_l);
}

uint32_t __attribute__((weak)) assembleStoreSignature(uint8_t store_active,
						      uint8_t asi,
						      uint8_t byte_mask,
						      uint8_t double_word_write,
						      uint32_t addr,
						      uint32_t word_high,
						      uint32_t word_low)
{
	(void) store_active;
	(void) asi;
	(void) byte_mask;
	(void) double_word_write;
	return (addr ^ word_high ^ word_low);
}

uint32_t __attribute__((weak)) register_port(char* id, int pipe_width, int is_input)
{
	(void) id;
	(void) pipe_width;
	(void) is_input;
	return 0;
}

uint32_t __attribute__((weak)) register_pipe(char* pipe_name, int pipe_depth, int pipe_width, int pipe_mode)
{
	(void) pipe_name;
	(void) pipe_depth;
	(void) pipe_width;
	(void) pipe_mode;
	return 0;
}

ThreadState* __attribute__((weak)) makeThreadState(uint32_t core_id,
						   uint32_t thread_id,
						   int isa_mode,
						   int bp_table_size,
						   int report_traps,
						   uint32_t init_pc)
{
	ThreadState* s = (ThreadState*) calloc(1, sizeof(ThreadState));
	if (s == NULL)
	{
		return NULL;
	}
	init_ajit_thread(s, core_id, thread_id, isa_mode, bp_table_size, report_traps, init_pc);
	return s;
}

ThreadState* __attribute__((weak)) start_ajit_thread(ThreadState* s)
{
	return s;
}

hwServerState* __attribute__((weak)) start_hw_server(uint32_t core_id, uint32_t cpu_id)
{
	(void) core_id;
	(void) cpu_id;
	return 0;
}
