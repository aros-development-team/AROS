/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Allocate a CPU affinity mask.
*/

#include <aros/config.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#include <kernel_base.h>

#include "kernel_riscv.h"

#include <proto/kernel.h>

/* See rom/kernel/alloccpumask.c for documentation */

AROS_LH0(void *, KrnAllocCPUMask,
        struct KernelBase *, KernelBase, 42, Kernel)
{
    AROS_LIBFUNC_INIT

#if defined(__AROSEXEC_SMP__)
    /* A fixed size, so a mask never depends on how many harts are up */
    return AllocMem(RISCV_CPUMASK_WORDS * sizeof(ULONG), MEMF_CLEAR | MEMF_PUBLIC);
#else
    return NULL;
#endif

    AROS_LIBFUNC_EXIT
}
