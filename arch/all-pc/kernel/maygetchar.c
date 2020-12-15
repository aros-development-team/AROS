/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include <proto/kernel.h>

#include <bootconsole.h>

/* See rom/kernel/maygetchar.c for documentation */
AROS_LH0(int, KrnMayGetChar,
    struct KernelBase *, KernelBase, 26, Kernel)
{
    AROS_LIBFUNC_INIT

    if (!con_CanGetc())
       return -1;

    return con_Getc();

    AROS_LIBFUNC_EXIT
}
