/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>

#include <hardware/mx51_uart.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include <proto/kernel.h>

/* See rom/kernel/maygetchar.c for documentation */

AROS_LH0(int, KrnMayGetChar,
    struct KernelBase *, KernelBase, 26, Kernel)
{
    AROS_LIBFUNC_INIT

    volatile MX51_UART * const uart = (MX51_UART *)UART1_BASE_ADDR;

    if (uart->USR2 & UART_USR2_RDR)
        return uart->URXD;
    else
        return -1;

    AROS_LIBFUNC_EXIT
}
