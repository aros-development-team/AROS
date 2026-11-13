/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Free a CPU affinity mask.
*/

#include <aros/config.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#include <kernel_base.h>

#include "kernel_riscv.h"

#include <proto/kernel.h>

/* See rom/kernel/freecpumask.c for documentation */

AROS_LH1(void *, KrnFreeCPUMask,
        AROS_LHA(void *, mask, A0),
        struct KernelBase *, KernelBase, 43, Kernel)
{
    AROS_LIBFUNC_INIT

#if defined(__AROSEXEC_SMP__)
    /* NULL is not a mask and the TASKAFFINITY_* values are sentinels */
    if (mask && ((IPTR)mask != TASKAFFINITY_ANY) &&
        ((IPTR)mask != TASKAFFINITY_ALL_BUT_SELF))
        FreeMem(mask, RISCV_CPUMASK_WORDS * sizeof(ULONG));
#else
    (void)mask;
#endif

    return NULL;

    AROS_LIBFUNC_EXIT
}
