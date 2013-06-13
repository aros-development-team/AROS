/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id:$

    Desc: kernel_debug.c
    Lang: english
*/

#include <exec/types.h>
#include <aros/kernel.h>
#include <inttypes.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include <hardware/mx51_uart.h>

static volatile MX51_UART * const uart = (MX51_UART *)UART1_BASE_ADDR;

static inline void waitBusy()
{
    while((uart->USR2 & UART_USR2_TXDC) == 0);
}

static inline void putByte(uint8_t chr)
{
    if (chr == '\n')
        uart->UTXD = '\r';
    uart->UTXD = chr;
}

void (*_KrnPutC)(char) = NULL;

void krnSerPutC(int chr)
{
	putByte(chr);
	waitBusy();
}

int krnPutC(int chr, struct KernelBase *KernelBase)
{
    if (chr == 0x03)
        _KrnPutC = NULL;
    else
    {
        if (_KrnPutC)
            _KrnPutC(chr);

        krnSerPutC(chr);
    }

    return 0;
}
