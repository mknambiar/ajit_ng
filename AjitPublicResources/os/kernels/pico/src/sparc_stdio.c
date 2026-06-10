#include "sparc_stdio.h"
#include "CommonSerialRoutines.h"

// Legacy pico stdio API, implemented on top of the serial device so existing
// validation tests can compile unchanged in the SITAR flow.

static int serial_stdio_ready = 0;

static void ensure_serial_stdio_ready(void)
{
	if(!serial_stdio_ready)
	{
		enableTx();
		enableRx();
		serial_stdio_ready = 1;
	}
}

//Indicate SUCCESS and HALT processor
void halt(void)
{
	__asm__ __volatile__("  mov %%g0, %%psr \n"
			     "	nop	\n"
			     "	nop	\n"
			     "	nop	\n"
			     "  ta 0    \n\t"
				:
				:
				: "cc");
}

void putChar(char val)
{
	ensure_serial_stdio_ready();
	sendAChar(val);
}

void putString(char word[])
{
	ensure_serial_stdio_ready();
	sendAString(word);
}

void putInt(int val)
{
	ensure_serial_stdio_ready();
	sendAUint((uint32_t) val);
}

void putFloat(float val)
{
	union { float f; uint32_t u; } conv;
	conv.f = val;
	ensure_serial_stdio_ready();
	sendAUint(conv.u);
}

void putDouble(double val)
{
	union { double d; uint64_t u; } conv;
	uint32_t hi;
	uint32_t lo;
	conv.d = val;
	hi = (uint32_t) (conv.u >> 32);
	lo = (uint32_t) conv.u;
	ensure_serial_stdio_ready();
	sendAUint(hi);
	sendAUint(lo);
}

char getChar()
{
	ensure_serial_stdio_ready();
	return getAChar();
}

int getInt()
{
	ensure_serial_stdio_ready();
	return (int) getAUint();
}

float getFloat()
{
	union { float f; uint32_t u; } conv;
	ensure_serial_stdio_ready();
	conv.u = getAUint();
	return conv.f;
}

double getDouble()
{
	union { double d; uint64_t u; } conv;
	uint32_t hi;
	uint32_t lo;
	ensure_serial_stdio_ready();
	hi = getAUint();
	lo = getAUint();
	conv.u = (((uint64_t) hi) << 32) | (uint64_t) lo;
	return conv.d;
}

void getString(char* str)
{
	int i = 0;
	char c;
	ensure_serial_stdio_ready();
	while(1)
	{
		c = getAChar();
		str[i++] = c;
		if(c == '\0')
			break;
	}
}
