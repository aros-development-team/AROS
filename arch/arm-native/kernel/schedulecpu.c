/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: arm-native KrnScheduleCPU - send IPI_SCHEDULE to a CPU mask.
*/

#include <exec/tasks.h>

#include <aros/kernel.h>
#include <aros/libcall.h>

#include "kernel_base.h"
#include "kernel_cpu.h"
#include "kernel_ipi.h"

AROS_LH1(void, KrnScheduleCPU,
        AROS_LHA(void *, cpu_mask, A0),
        struct KernelBase *, KernelBase, 47, Kernel)
{
    AROS_LIBFUNC_INIT

#if defined(__AROSEXEC_SMP__)
    if (cpu_mask && __arm_arosintern.ARMI_SendIPI)
    {
        /*
         * On arm-native cpumask_t is unsigned int, so the affinity buffer
         * is a small bitmap with the lowest 32 bits indexing CPUs 0-31.
         * Pi 2/3/4 have at most 4 cores - one word is plenty.
         *
         * TASKAFFINITY_ANY / TASKAFFINITY_ALL_BUT_SELF are sentinels
         * (not buffer pointers) and must not be dereferenced.
         */
        int srcCpu = GetCPUNumber();
        uint32_t mask;

        if ((IPTR)cpu_mask == TASKAFFINITY_ANY)
            mask = 0xf;
        else if ((IPTR)cpu_mask == TASKAFFINITY_ALL_BUT_SELF)
            mask = 0xf & ~(1U << srcCpu);
        else
            mask = *(uint32_t *)cpu_mask;

        __arm_arosintern.ARMI_SendIPI((IPI_SCHEDULE & 0x0fffffff) | (srcCpu << 28),
                                      0, mask);
    }
#else
    (void)cpu_mask;
#endif

    AROS_LIBFUNC_EXIT
}
