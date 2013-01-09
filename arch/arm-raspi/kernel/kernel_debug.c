/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <inttypes.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#define UART0_BASE   0x20201000
#define UART_DR     (0x00)
#define UART_FR     (0x18)

void (*_KrnPutC)(char *) = NULL;

inline void waitSerOUT(volatile uint32_t *uart)
{
    while(1)
    {
       if ((uart[UART_FR] & 0x20) == 0) break;
    }
}

inline void putByte(uint8_t chr)
{
    volatile uint32_t *uart = (uint32_t *)UART0_BASE;

    waitSerOUT(uart);

    uart[UART_DR] = chr;
    if (chr == '\n')
        uart[UART_DR] = '\r';
}

int krnPutC(int chr, struct KernelBase *KernelBase)
{
    if (_KrnPutC)
        _KrnPutC(chr);

    putByte(chr);
}
