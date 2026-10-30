/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Test whether a CPU is in an affinity mask.
*/

#include <aros/config.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#include <kernel_base.h>

#include "kernel_riscv.h"

#include <proto/kernel.h>

/* See rom/kernel/cpuinmask.c for documentation */

AROS_LH2(BOOL, KrnCPUInMask,
        AROS_LHA(uint32_t, id, D0),
        AROS_LHA(void *, mask, A0),
        struct KernelBase *, KernelBase, 46, Kernel)
{
    AROS_LIBFUNC_INIT

#if defined(__AROSEXEC_SMP__)
    /* A NULL mask is invalid and selects no CPU */
    if (!mask || (id >= RISCV_MAXCPUS))
        return FALSE;

    if ((IPTR)mask == TASKAFFINITY_ANY)
        return TRUE;
    if ((IPTR)mask == TASKAFFINITY_ALL_BUT_SELF)
        return (id != krnRVCPUNumber());

    return (((ULONG *)mask)[id / 32] & (1UL << (id % 32))) ? TRUE : FALSE;
#else
    (void)id;
    (void)mask;

    return TRUE;
#endif

    AROS_LIBFUNC_EXIT
}
