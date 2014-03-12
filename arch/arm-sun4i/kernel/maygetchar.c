/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/kernel.h>
#include <kernel_base.h>
#include <kernel_debug.h>
#include <proto/kernel.h>

#include <hardware/sun4i/uart.h>
#include <hardware/sun4i/platform.h>

/* See rom/kernel/maygetchar.c for documentation */

AROS_LH0(int, KrnMayGetChar,
    struct KernelBase *, KernelBase, 26, Kernel)
{
    AROS_LIBFUNC_INIT

	return -1;

    AROS_LIBFUNC_EXIT
}
