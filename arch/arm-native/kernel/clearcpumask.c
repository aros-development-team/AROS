/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: arm-native KrnClearCPUMask - zero the CPU affinity bitmap.
*/

#include <exec/tasks.h>

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

AROS_LH1(void, KrnClearCPUMask,
        AROS_LHA(void *, mask, A0),
        struct KernelBase *, KernelBase, 44, Kernel)
{
    AROS_LIBFUNC_INIT

    if (mask && (IPTR)mask != TASKAFFINITY_ANY)
        *(uint32_t *)mask = 0;

    AROS_LIBFUNC_EXIT
}
