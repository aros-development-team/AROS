/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <stdio.h>
#include <inttypes.h>

#include <hardware/sun4i/platform.h>
#include <hardware/sun4i/uart.h>

static inline void waitBusy() {
	volatile struct UART *UARTDEBUG;
	UARTDEBUG = SUN4I_UARTDEBUG_BASE;

	while ((UARTDEBUG->LSR & LSR_THRE) == 0);
}

static inline void putByte(uint8_t chr) {
	volatile struct UART *UARTDEBUG;
	UARTDEBUG = SUN4I_UARTDEBUG_BASE;

	UARTDEBUG->THR = (uint32_t) chr;
//	if (chr == '\n') {
//		UARTDEBUG->THR = (uint32_t) '\r';
//	}
}

void putBytes(const char *str) {
	while(*str) {
		waitBusy();
		putByte(*str++);
	}
}

void kprintf(const char *format, ...) {
	char tmpbuf[512];
	char *out = tmpbuf;

	va_list vp;

	va_start(vp, format);
	vsnprintf(tmpbuf, 511, format, vp);
	va_end(vp);

	putBytes(out);
}
