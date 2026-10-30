/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Clear a CPU affinity mask.
*/

#include <aros/config.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#include <kernel_base.h>

#include "kernel_riscv.h"

#include <proto/kernel.h>

/* See rom/kernel/clearcpumask.c for documentation */

AROS_LH1(void, KrnClearCPUMask,
        AROS_LHA(void *, mask, A0),
        struct KernelBase *, KernelBase, 44, Kernel)
{
    AROS_LIBFUNC_INIT

#if defined(__AROSEXEC_SMP__)
    int i;

    if (!mask || ((IPTR)mask == TASKAFFINITY_ANY) ||
        ((IPTR)mask == TASKAFFINITY_ALL_BUT_SELF))
        return;

    for (i = 0; i < RISCV_CPUMASK_WORDS; i++)
        ((ULONG *)mask)[i] = 0;
#else
    (void)mask;
#endif

    AROS_LIBFUNC_EXIT
}
