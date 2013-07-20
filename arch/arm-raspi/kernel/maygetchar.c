/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>

#include <asm/bcm2835.h>
#include <hardware/pl011uart.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include <proto/kernel.h>

/* See rom/kernel/maygetchar.c for documentation */

AROS_LH0(int, KrnMayGetChar,
    struct KernelBase *, KernelBase, 26, Kernel)
{
    AROS_LIBFUNC_INIT

    if ((*(volatile uint32_t *)(PL011_0_BASE + PL011_FR) & PL011_FR_RXFE) == 0)
        return (int)*(volatile uint32_t *)(PL011_0_BASE + PL011_DR);

    return -1;

    AROS_LIBFUNC_EXIT
}
