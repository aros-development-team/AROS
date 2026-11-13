/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: arm-native KrnCPUInMask - test whether a CPU is in the mask.
*/

#include <exec/tasks.h>

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

AROS_LH2(BOOL, KrnCPUInMask,
        AROS_LHA(uint32_t, id, D0),
        AROS_LHA(void *, mask, A0),
        struct KernelBase *, KernelBase, 46, Kernel)
{
    AROS_LIBFUNC_INIT

    /* A NULL affinity means none was assigned - treat as unrestricted
     * (run anywhere) rather than unschedulable. */
    if (!mask)
        return TRUE;

    /* TASKAFFINITY_ANY ((IPTR)-1) is a sentinel meaning "any CPU" -
     * not a buffer pointer. Treat it as a match for every CPU. */
    if ((IPTR)mask == TASKAFFINITY_ANY)
        return TRUE;

    return (((uint32_t *)mask)[id >> 5] & (1U << (id & 31))) ? TRUE : FALSE;

    AROS_LIBFUNC_EXIT
}
