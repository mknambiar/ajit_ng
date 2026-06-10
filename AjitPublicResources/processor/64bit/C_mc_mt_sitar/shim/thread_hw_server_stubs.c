#include <stdint.h>

typedef struct _ThreadState ThreadState;

// Weak no-op stubs for Sitar mode.
// If real ThreadHWserverInterface.c is linked later, its strong symbols override these.
void __attribute__((weak)) thread_enable_HW_server(ThreadState* thread_state, int gdb_flag, int doval_flag)
{
	(void) thread_state;
	(void) gdb_flag;
	(void) doval_flag;
}

void __attribute__((weak)) thread_disable_HW_server(ThreadState* thread_state)
{
	(void) thread_state;
}

int __attribute__((weak)) thread_hw_server_enabled(ThreadState* thread_state)
{
	(void) thread_state;
	return 0;
}

int __attribute__((weak)) thread_gdb_flag(ThreadState* thread_state)
{
	(void) thread_state;
	return 0;
}

int __attribute__((weak)) thread_doval_flag(ThreadState* thread_state)
{
	(void) thread_state;
	return 0;
}

void __attribute__((weak)) inform_HW_server(ThreadState* s, uint8_t message_type, uint32_t address)
{
	(void) s;
	(void) message_type;
	(void) address;
}
