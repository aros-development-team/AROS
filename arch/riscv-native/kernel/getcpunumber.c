/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Logical number of the calling CPU (boot hart = 0).
*/

#include <aros/config.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#include <kernel_base.h>

#include "kernel_riscv.h"

#include <proto/kernel.h>

/* See rom/kernel/getcpunumber.c for documentation */

AROS_LH0(unsigned int, KrnGetCPUNumber,
         struct KernelBase *, KernelBase, 41, Kernel)
{
    AROS_LIBFUNC_INIT

    /*
     * The logical number, not the hart id: CPU masks are indexed by it,
     * and the boot hart need not be hart 0.
     */
    return krnRVCPUNumber();

    AROS_LIBFUNC_EXIT
}
