/*
    Copyright © 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include "kernel_intern.h"

#include <proto/kernel.h>
#include <stdint.h>

/* See rom/kernel/maygetchar.c for documentation */

AROS_LH0(int, KrnMayGetChar,
    struct KernelBase *, KernelBase, 26, Kernel)
{
    AROS_LIBFUNC_INIT

    if (__arm_arosintern.ARMI_SerGetChar)
        return __arm_arosintern.ARMI_SerGetChar();

    return -1;

    AROS_LIBFUNC_EXIT
}
