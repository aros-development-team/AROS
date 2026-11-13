/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: aarch64-native KrnAllocCPUMask - allocate a CPU affinity bitmap.
*/

#include <proto/exec.h>

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/* raspi tops out at 4 cores, so one word covers any affinity. */
#define AARCH64_CPUMASK_BYTES   sizeof(uint32_t)

AROS_LH0(void *, KrnAllocCPUMask,
        struct KernelBase *, KernelBase, 42, Kernel)
{
    AROS_LIBFUNC_INIT

    return AllocMem(AARCH64_CPUMASK_BYTES, MEMF_CLEAR | MEMF_PUBLIC);

    AROS_LIBFUNC_EXIT
}
