/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id:$

    Desc: kernel_debug.c
    Lang: english
*/

#include <aros/kernel.h>
#include <inttypes.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#define UART1_BASE_ADDR		0x73fbc000

#define ONEMS	(0xb0/4)
#define UBIR	(0xa4/4)
#define UBMR	(0xa8/4)
#define UCR2	(0x84/4)

inline void waitBusy()
{
	volatile uint32_t *uart = (uint32_t *)UART1_BASE_ADDR;
	while(!(uart[0x98 / 4] & (1 << 3)));
}

inline void putByte(uint8_t chr)
{
	volatile uint32_t *uart = (uint32_t *)UART1_BASE_ADDR;
	uart[0x10] = chr;
	if (chr == '\n')
		uart[0x10] = '\r';
}

int krnPutC(int chr, struct KernelBase *KernelBase)
{
	putByte(chr);
	waitBusy();
}
