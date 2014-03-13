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

	while ((UARTDEBUG->lsr & LSR_THRE) == 0);
}

static inline void putByte(uint8_t chr) {
	volatile struct UART *UARTDEBUG;
	UARTDEBUG = SUN4I_UARTDEBUG_BASE;

	UARTDEBUG->thr = (uint32_t) chr;
//	if (chr == '\n') {
//		UARTDEBUG->thr = (uint32_t) '\r';
//	}
}
void putBytes(const char *str) {
	while(*str) {
		waitBusy();
		putByte(*str++);
	}
}

static char tmpbuf[512];
void kprintf(const char *format, ...) {
	char *out = tmpbuf;
	va_list vp;

	va_start(vp, format);
	vsnprintf(tmpbuf, 511, format, vp);
	va_end(vp);

	putBytes(out);
}
