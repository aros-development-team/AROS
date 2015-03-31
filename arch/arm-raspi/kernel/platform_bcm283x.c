/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include "kernel_intern.h"
#include "kernel_arm.h"

static void bcm283x_init(void)
{
#if (0)
    // TODO:
    // How to identify broadcom IP's?
    // Expose this as a seprate subsystem (like PCI?)
    D(bug("[Kernel] Integrated Peripherals -:\n"));
    for (ptr = ARM_PERIIOBASE; ptr < (ARM_PERIIOBASE + ARM_PERIIOSIZE); ptr += ARM_PRIMECELLPERISIZE)
    {
        unsigned int perihreg = (*(volatile unsigned int *)(ptr + 0xFF0) & 0xFF) | (*(volatile unsigned int *)(ptr + 0xFF4) & 0xFF) << 8 | (*(volatile unsigned int *)(ptr + 0xFF8) & 0xFF) << 16 | (*(volatile unsigned int *)(ptr + 0xFFC) & 0xFF) << 24;
        if (perihreg == ARM_PRIMECELLID)
        {
            perihreg = (*(volatile unsigned int *)(ptr + 0xFE0) & 0xFF) | (*(volatile unsigned int *)(ptr + 0xFE4) & 0xFF) << 8 | (*(volatile unsigned int *)(ptr + 0xFE8) & 0xFF) << 16 | (*(volatile unsigned int *)(ptr + 0xFEC) & 0xFF) << 24;
            unsigned int manu = (perihreg & (0x7F << 12)) >> 12;
            unsigned int prod = (perihreg & 0xFFF);
            unsigned int rev = (perihreg & (0xF << 20)) >> 20;
            unsigned int config = (perihreg & (0x7F << 24)) >> 24;
            D(bug("[Kernel]           0x%p: manu %x, prod %x, rev %d, config %d\n", ptr, manu, prod, rev, config));
        }
/*        else
        {
            if (perihreg)
            {
                D(bug("[Kernel] PlatformPostInit:           0x%p: PrimeCellID != %08x\n", ptr, perihreg));
            }
        }*/
    }
#endif
}

static void bcm283x_toggle_led(void)
{
    
}

static IPTR bcm283x_probe(struct KernelBase *KernelBase)
{
    return TRUE;
}

ADD2SET(bcm283x_probe, ARMPLATFORMS, 0)
