/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Allocate only the CPU context state supported by this m68k CPU.
*/

#include <stddef.h>

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_cpu.h>

#include <proto/kernel.h>

AROS_LH0(void *, KrnCreateContext,
         struct KernelBase *, KernelBase, 18, Kernel)
{
    AROS_LIBFUNC_INIT

    ULONG size;

    /* The bootstrap context predates CPU detection and must fit every CPU. */
    if (SysBase->ThisTask == NULL || (SysBase->AttnFlags & AFF_68080))
        size = sizeof(struct AROSCPUContext);
    else if (SysBase->AttnFlags & AFF_FPU)
        size = offsetof(struct AROSCPUContext, ammx);
    else
        size = sizeof(struct ExceptionContext);

    return AllocVec(size, MEMF_CLEAR);

    AROS_LIBFUNC_EXIT
}
