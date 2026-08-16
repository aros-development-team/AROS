/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.

    Desc: arm-native KrnGetCPUMask - set the bit for a CPU in the mask buffer.
*/

#include <exec/tasks.h>

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

AROS_LH2(void, KrnGetCPUMask,
        AROS_LHA(uint32_t, id, D0),
        AROS_LHA(void *, mask, A0),
        struct KernelBase *, KernelBase, 45, Kernel)
{
    AROS_LIBFUNC_INIT

    if (mask && (IPTR)mask != TASKAFFINITY_ANY)
        ((uint32_t *)mask)[id >> 5] |= (1U << (id & 31));

    AROS_LIBFUNC_EXIT
}
