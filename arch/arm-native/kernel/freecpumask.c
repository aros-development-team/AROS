/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: arm-native KrnFreeCPUMask - release a CPU affinity bitmap.
*/

#include <exec/tasks.h>

#include <proto/exec.h>

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

AROS_LH1(void *, KrnFreeCPUMask,
        AROS_LHA(void *, mask, A0),
        struct KernelBase *, KernelBase, 43, Kernel)
{
    AROS_LIBFUNC_INIT

    /* TASKAFFINITY_ANY is a sentinel, not an allocated buffer. */
    if (mask && (IPTR)mask != TASKAFFINITY_ANY)
        FreeMem(mask, sizeof(uint32_t));

    return NULL;

    AROS_LIBFUNC_EXIT
}
