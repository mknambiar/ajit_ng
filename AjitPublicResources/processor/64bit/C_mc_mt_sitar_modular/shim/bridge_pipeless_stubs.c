#include <stdint.h>
#include "ajit_memory_shim.h"

// Local subset from RequestTypeValues.h to avoid external include-path dependency.
#define REQUEST_TYPE_IFETCH          0
#define REQUEST_TYPE_READ            1
#define REQUEST_TYPE_WRITE           2
#define REQUEST_TYPE_WRFSRFAR        4
#define REQUEST_TYPE_CCU_CACHE_READ  5
#define REQUEST_TYPE_CCU_CACHE_WRITE 6

static int is_read_req(uint8_t request_type)
{
	uint8_t rt = (request_type & 0x3f);
	return ((rt == REQUEST_TYPE_READ) ||
		(rt == REQUEST_TYPE_IFETCH) ||
		(rt == REQUEST_TYPE_CCU_CACHE_READ));
}

static int is_write_req(uint8_t request_type)
{
	uint8_t rt = (request_type & 0x3f);
	return ((rt == REQUEST_TYPE_WRITE) ||
		(rt == REQUEST_TYPE_CCU_CACHE_WRITE) ||
		(rt == REQUEST_TYPE_WRFSRFAR));
}

int __attribute__((weak)) sysMemBusRequest(int core_id,
                                           int thread_id,
                                           uint8_t request_type,
                                           uint8_t byte_mask,
                                           uint32_t addr,
                                           uint64_t data64,
                                           uint64_t* rdata)
{
	(void) core_id;
	(void) thread_id;

	uint32_t aligned_addr = (addr & 0xfffffff8u);

	if (is_read_req(request_type)) {
		uint64_t rd = 0;
		ajit_shim_accessMemU64(1, 0xff, aligned_addr, 0, &rd);
		if (rdata) {
			*rdata = rd;
		}
		return 1;
	}

	if (is_write_req(request_type)) {
		ajit_shim_accessMemU64(0, byte_mask, aligned_addr, data64, 0);
		if (rdata) {
			*rdata = 0;
		}
		return 1;
	}

	// Barrier/no-op style requests complete immediately in this phase.
	if (rdata) {
		*rdata = 0;
	}
	return 1;
}
