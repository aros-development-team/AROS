/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#include <exec/types.h>
#include <aros/kernel.h>
#include <inttypes.h>

#include <kernel_base.h>
#include <kernel_debug.h>

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

void (*_KrnPutC)(char) = NULL;

void krnSerPutC(int chr) {
	waitBusy();
	putByte(chr);
}

int krnPutC(int chr, struct KernelBase *KernelBase) {
    if (chr == 0x03) {
        _KrnPutC = NULL;
    } else {
        if (_KrnPutC) {
            _KrnPutC(chr);
		}
        krnSerPutC(chr);
    }

    return 0;
}
