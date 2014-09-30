/*
    Copyright © 2009-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <inttypes.h>
#include <stdio.h>
#include "serialdebug.h"

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

void putBytes(const char *str)
{
	while(*str)
	{
		putByte(*str++);
		waitBusy();
	}
}

static char tmpbuf[512];

void kprintf(const char *format, ...)
{
	char *out = tmpbuf;
	va_list vp;

	va_start(vp, format);
	vsnprintf(tmpbuf, 511, format, vp);
	va_end(vp);

	putBytes(out);
}
