/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include <asm/amcc440.h>
#include <asm/io.h>

#include <proto/kernel.h>

/* See rom/kernel/maygetchar.c for documentation */

AROS_LH0(int, KrnMayGetChar,
    struct KernelBase *, KernelBase, 26, Kernel)
{
    AROS_LIBFUNC_INIT

    if (inb(UART0_LSR) & UART_LSR_DR)
        return inb(UART0_RBR);

    return -1;

    AROS_LIBFUNC_EXIT
}
