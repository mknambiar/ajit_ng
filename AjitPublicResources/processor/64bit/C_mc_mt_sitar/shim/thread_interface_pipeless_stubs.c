#include <stdint.h>
#include "AjitThread.h"
#include "ajit_memory_shim.h"
#include "ajit_thread_bridge.h"

// Pipe-free thread-interface behavior for Sitar bring-up.
// These are weak so real implementations can override later.

uint8_t __attribute__((weak)) getBpIRL(ThreadState* s)
{
	{
	uint32_t threads_per_core = (uint32_t) ajit_thread_bridge_get_threads_per_core();
	if (threads_per_core == 0u) {
		threads_per_core = 2u;
	}
	int slot_id = (int) ((s->core_id * threads_per_core) + s->thread_id);
	ajit_shim_sample_irq(slot_id);
	return ajit_shim_get_irq(slot_id);
	}
}

uint8_t __attribute__((weak)) getBpReset(ThreadState* s)
{
	(void) s;
	// Keep reset deasserted so the thread can leave RESET mode.
	return 0;
}

void __attribute__((weak)) setPbError(ThreadState* s, uint8_t val)
{
	(void) s;
	(void) val;
}

void __attribute__((weak)) setPbBlockLdstWord(ThreadState* s, uint8_t val)
{
	(void) s;
	(void) val;
}

void __attribute__((weak)) setPbBlockLdstByte(ThreadState* s, uint8_t val)
{
	(void) s;
	(void) val;
}

uint8_t __attribute__((weak)) getPbBlockLdstWord(ThreadState* s)
{
	(void) s;
	return 0;
}

uint8_t __attribute__((weak)) getPbBlockLdstByte(ThreadState* s)
{
	(void) s;
	return 0;
}
