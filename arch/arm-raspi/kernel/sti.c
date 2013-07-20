/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

#include <proto/kernel.h>

/* See rom/kernel/sti.c for documentation */

AROS_LH0I(void, KrnSti,
    struct KernelBase *, KernelBase, 10, Kernel)
{
    AROS_LIBFUNC_INIT

    krnSysCall(SC_STI);

    AROS_LIBFUNC_EXIT
}
