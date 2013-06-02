/*
    Copyright � 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <inttypes.h>

#include <asm/bcm2835.h>
#include <hardware/pl011uart.h>

#include <kernel_base.h>
#include <kernel_debug.h>

void (*_KrnPutC)(char) = NULL;

inline void krnWaitSerOut()
{
    while(1)
    {
       if ((*(volatile uint32_t *)(PL011_0_BASE + PL011_FR) & PL011_FR_TXFF) == 0) break;
    }
}


inline void krnSerPutC(uint8_t chr)
{
    krnWaitSerOut();

    if (chr == '\n')
        *(volatile uint32_t *)(PL011_0_BASE + PL011_DR) = '\r';
    *(volatile uint32_t *)(PL011_0_BASE + PL011_DR) = chr;
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
}
