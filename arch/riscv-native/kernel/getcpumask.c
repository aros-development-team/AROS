/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Add a CPU to an affinity mask.
*/

#include <aros/config.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#include <kernel_base.h>

#include "kernel_riscv.h"

#include <proto/kernel.h>

/* See rom/kernel/getcpumask.c for documentation */

AROS_LH2(void, KrnGetCPUMask,
        AROS_LHA(uint32_t, id, D0),
        AROS_LHA(void *, mask, A0),
        struct KernelBase *, KernelBase, 45, Kernel)
{
    AROS_LIBFUNC_INIT

#if defined(__AROSEXEC_SMP__)
    if (!mask || ((IPTR)mask == TASKAFFINITY_ANY) ||
        ((IPTR)mask == TASKAFFINITY_ALL_BUT_SELF) || (id >= RISCV_MAXCPUS))
        return;

    /* ORs the CPU in, as the other ports do - callers build sets with it */
    ((ULONG *)mask)[id / 32] |= 1UL << (id % 32);
#else
    (void)id;
    (void)mask;
#endif

    AROS_LIBFUNC_EXIT
}
