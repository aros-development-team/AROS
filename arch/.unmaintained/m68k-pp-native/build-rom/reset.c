#include <asm/registers.h>
#include <gfx.h>

extern void main_init(void * memory, ULONG memSize);

void aros_reset(void)
{
	/*
	 * First parameter is memory start, 2nd is size of memory.
	 */
	main_init((void *)0x400,0x90000-0x400);
}
