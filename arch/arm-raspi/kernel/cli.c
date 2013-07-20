/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_syscall.h>

#include <proto/kernel.h>

/* See rom/kernel/cli.c for documentation */

AROS_LH0I(void, KrnCli,
    struct KernelBase *, KernelBase, 9, Kernel)
{
    AROS_LIBFUNC_INIT

    krnSysCall(SC_CLI);

    AROS_LIBFUNC_EXIT
}
