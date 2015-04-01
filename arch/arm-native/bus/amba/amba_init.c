/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/kernel.h>

int ambabus_init()
{
    IPTR __arm_periiobase;

    if ((__arm_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase)) != NULL)
    {
        D(bug("[AMBA] %s: Integrated Peripherals -:\n", __PRETTY_FUNCTION__));
        for (ptr = __arm_periiobase; ptr < (__arm_periiobase + ARM_PERIIOSIZE); ptr += ARM_PRIMECELLPERISIZE)
        {
            unsigned int perihreg = (*(volatile unsigned int *)(ptr + 0xFF0) & 0xFF) | (*(volatile unsigned int *)(ptr + 0xFF4) & 0xFF) << 8 | (*(volatile unsigned int *)(ptr + 0xFF8) & 0xFF) << 16 | (*(volatile unsigned int *)(ptr + 0xFFC) & 0xFF) << 24;
            if (perihreg == ARM_PRIMECELLID)
            {
                perihreg = (*(volatile unsigned int *)(ptr + 0xFE0) & 0xFF) | (*(volatile unsigned int *)(ptr + 0xFE4) & 0xFF) << 8 | (*(volatile unsigned int *)(ptr + 0xFE8) & 0xFF) << 16 | (*(volatile unsigned int *)(ptr + 0xFEC) & 0xFF) << 24;
                unsigned int manu = (perihreg & (0x7F << 12)) >> 12;
                unsigned int prod = (perihreg & 0xFFF);
                unsigned int rev = (perihreg & (0xF << 20)) >> 20;
                unsigned int config = (perihreg & (0x7F << 24)) >> 24;
                D(bug("[AMBA] %s:          0x%p: manu %x, prod %x, rev %d, config %d\n", __PRETTY_FUNCTION__, ptr, manu, prod, rev, config));
            }
    /*        else
            {
                if (perihreg)
                {
                    D(bug("[AMBA] %s:           0x%p: PrimeCellID != %08x\n", __PRETTY_FUNCTION__, ptr, perihreg));
                }
            }*/
        }
    }
    return 0;
}
