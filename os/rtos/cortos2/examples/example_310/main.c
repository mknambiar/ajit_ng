#include <math.h>
#include <cortos.h>

#include <stdint.h>
#include <stdio.h>
#include <setjmp.h>
#include <ajit_access_routines.h>
#include <core_portme.h>
#include <ajit_mt_irc.h>
#include <ajit_generic_sys_calls.h>

volatile int exit_flag = 0;
static int sw_trap_fired = 0;
// Keep timer disabled while debugging serial IRQs.

static void make_PIL_0(void)
{
	__asm__ __volatile__(
		"rd %%psr, %%g1\n\t"
		"andn %%g1, 0xF00, %%g1\n\t"
		"wr %%g1, 0, %%psr\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		:
		:
		: "g1"
	);
}

static void put_hex32(uint32_t v)
{
	static const char hex[] = "0123456789abcdef";
	for (int i = 7; i >= 0; --i) {
		uint8_t nib = (v >> (i * 4)) & 0xF;
		__ajit_serial_putchar_via_bypass__(hex[nib]);
	}
}

static void put_str(const char *s)
{
	while (*s) {
		__ajit_serial_putchar_via_bypass__(*s++);
	}
}

static inline uint32_t serial_ctrl_read_vmap_word(void)
{
	return *((volatile uint32_t*) ADDR_SERIAL_CONTROL_REGISTER);
}

static inline void serial_ctrl_write_vmap_word(uint32_t v)
{
	*((volatile uint32_t*) ADDR_SERIAL_CONTROL_REGISTER) = v;
}

static inline uint32_t serial1_ctrl_read_vmap_word(void)
{
	return *((volatile uint32_t*) ADDR_SERIAL_1_CONTROL_REGISTER);
}

static inline void serial1_ctrl_write_vmap_word(uint32_t v)
{
	*((volatile uint32_t*) ADDR_SERIAL_1_CONTROL_REGISTER) = v;
}

static inline uint8_t serial1_rx_read_vmap_byte(void)
{
	return *((volatile uint8_t*) ADDR_SERIAL_1_RX_REGISTER);
}

//
// Note: user defined, but will run in supervisor mode.
//
void my_serial_interrupt_handler() {

	uint32_t fp_regs[34];
	uint32_t fp_addr = (uint32_t) &(fp_regs[0]);

	__AJIT_SAVE_FP_REGS__ (fp_addr);

	//
	// Read the byte from the serial device using a user-level routine!
	// We could use the supervisor-level routine also here....
	//
	uint32_t B = __ajit_read_serial_rx_register_via_vmap__();

	__ajit_serial_putchar_via_bypass__('!');
	{
		uint32_t icr = readInterruptControlRegister(0,0);
		put_str(" I=");
		put_hex32(icr);
	}

	if(B == 'q') {
		exit_flag = 1;
	}

	__ajit_serial_putchar_via_bypass__(B);

	__AJIT_RESTORE_FP_REGS__ (fp_addr);
}

int main ()
{
	exit_flag = 0;
	sw_trap_fired = 0;

	// // Note: do not forget to initialize the specific
	// //       interrupt handlers.
	ajit_initialize_interrupt_handlers_to_null();
	ajit_set_interrupt_handler(12, &(my_serial_interrupt_handler));

	// enableInterruptControllerAndAllInterrupts(0,0);

	{
		uint32_t psr;
		ajit_sys_go_to_supervisor_mode(1);
		__AJIT_GET_PSR(psr);
		psr |= (1 << 5); // ET=1 (enable traps/interrupts)
		__asm__ __volatile__("wr %0, 0, %%psr\n\t" : : "r"(psr));
		__AJIT_NOP();
		__AJIT_NOP();
		__AJIT_NOP();
		make_PIL_0();
		__AJIT_GET_PSR(psr);
		__ajit_serial_putchar_via_bypass__('P');
		__ajit_serial_putchar_via_bypass__('=');
		put_hex32(psr);
		put_str(" PIL=");
		put_hex32((psr >> 8) & 0xF);
		put_str(" ET=");
		put_hex32((psr >> 5) & 0x1);
		put_str(" S=");
		put_hex32((psr >> 7) & 0x1);
		__ajit_serial_putchar_via_bypass__('\n');
	}

	__ajit_serial_configure_via_vmap__(1, 1, 1);
	__ajit_serial_set_baudrate_via_vmap__(115200, CLK_FREQUENCY);
	__ajit_serial_set_uart_reset_via_vmap__(0);
	serial_ctrl_write_vmap_word(TX_ENABLE | RX_ENABLE | RX_INTR_ENABLE);
	__ajit_write_serial_control_register_via_bypass__(TX_ENABLE | RX_ENABLE | RX_INTR_ENABLE);
	put_str(" SCv=");
	put_hex32(serial_ctrl_read_vmap_word());
	put_str(" SCb=");
	put_hex32(__ajit_read_serial_control_register_via_bypass__());
	serial1_ctrl_write_vmap_word(TX_ENABLE | RX_ENABLE | RX_INTR_ENABLE);
	put_str(" S1=");
	put_hex32(serial1_ctrl_read_vmap_word());
	__ajit_serial_putchar_via_bypass__('\n');
	__ajit_serial_putchar_via_bypass__('.');

	enableInterruptControllerAndAllInterrupts(0,0);
	{
		uint32_t icr = readInterruptControlRegister(0,0);
		uint32_t ctrl_vmap = serial_ctrl_read_vmap_word();
		uint32_t ctrl_bypass = __ajit_read_serial_control_register_via_bypass__();
		put_str(" ICR=");
		put_hex32(icr);
		put_str(" EN=");
		put_hex32(icr & 0x1);
		put_str(" MASK=");
		put_hex32((icr >> 1) & 0x7fff);
		put_str(" ACT=");
		put_hex32((icr >> 17) & 0x7fff);
		put_str(" V=");
		put_hex32(ctrl_vmap);
		put_str(" B=");
		put_hex32(ctrl_bypass);
		__ajit_serial_putchar_via_bypass__('\n');
	}

	// timer disabled for now

	unsigned int loop_count = 0;
	int rx_full_reported = 0;
	// uint32_t last_timer = 0xffffffff;

	while(1) {
		add_delay();
		loop_count++;
		if ((loop_count % 5000) == 0) {
			// Re-assert RX interrupt enable; it seems to get cleared elsewhere.
			serial_ctrl_write_vmap_word(TX_ENABLE | RX_ENABLE | RX_INTR_ENABLE);
			__ajit_write_serial_control_register_via_bypass__(TX_ENABLE | RX_ENABLE | RX_INTR_ENABLE);
			uint32_t ctrl_vmap = serial_ctrl_read_vmap_word();
			uint32_t ctrl_bypass = __ajit_read_serial_control_register_via_bypass__();
			if (!rx_full_reported && (ctrl_bypass & 0x10)) {
				__ajit_serial_putchar_via_bypass__('#');
				__ajit_serial_putchar_via_bypass__('V');
				__ajit_serial_putchar_via_bypass__('=');
				put_hex32(ctrl_vmap);
				__ajit_serial_putchar_via_bypass__(' ');
				__ajit_serial_putchar_via_bypass__('B');
				__ajit_serial_putchar_via_bypass__('=');
				put_hex32(ctrl_bypass);
				__ajit_serial_putchar_via_bypass__('\n');
				rx_full_reported = 1;
			}
			if (!sw_trap_fired && (ctrl_bypass & 0x10)) {
				sw_trap_fired = 1;
				cortos_trap(0x80);
			}
			uint32_t ctrl1_vmap = serial1_ctrl_read_vmap_word();
			if (ctrl1_vmap & 0x10) {
				__ajit_serial_putchar_via_bypass__('1');
				__ajit_serial_putchar_via_bypass__('#');
				uint32_t icr = readInterruptControlRegister(0,0);
				__ajit_serial_putchar_via_bypass__('I');
				__ajit_serial_putchar_via_bypass__('=');
				put_hex32(icr);
				__ajit_serial_putchar_via_bypass__(' ');
				__ajit_serial_putchar_via_bypass__('V');
				__ajit_serial_putchar_via_bypass__('=');
				put_hex32(ctrl1_vmap);
				__ajit_serial_putchar_via_bypass__(' ');
				__ajit_serial_putchar_via_bypass__('R');
				__ajit_serial_putchar_via_bypass__('=');
				put_hex32(serial1_rx_read_vmap_byte());
				__ajit_serial_putchar_via_bypass__('\n');
			}
		}
		if(exit_flag) {
			break;
		}
	}

	__ajit_serial_putchar_via_bypass__('\n');
	__ajit_serial_putchar_via_bypass__('B');
	__ajit_serial_putchar_via_bypass__('Y');
	__ajit_serial_putchar_via_bypass__('E');
	__ajit_serial_putchar_via_bypass__('\n');
	return(0);
}
