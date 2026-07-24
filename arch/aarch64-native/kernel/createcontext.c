/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Create an empty usable CPU context, AArch64 raspi version.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <aros/aarch64/cpucontext.h>

#include <kernel_base.h>
#include <kernel_objects.h>

#include "kernel_cpu.h"
#include "kernel_intern.h"

AROS_LH0(void *, KrnCreateContext,
         struct KernelBase *, KernelBase, 18, Kernel)

{
    AROS_LIBFUNC_INIT

    struct ExceptionContext *ctx;

    /*
     * Common data block and FPU data block are allocated in one chunk
     * (see cpu_Init for the sizing); fpuContext points at the 16-byte
     * aligned tail. Tasks run at EL1t on this port, so the initial
     * PSTATE must say so -- a zeroed cpsr would be EL0t and the first
     * dispatch ERET would fault (EL0 is unusable: no domains, and
     * EL0-writable pages are forced PXN).
     */
    ctx = krnAllocCPUContext();
    if (ctx)
    {
        ctx->fpuContext = (APTR)(((IPTR)ctx + sizeof(struct ExceptionContext) + 15) & ~15);
        ctx->Flags      = ECF_FPU;
        ctx->cpsr       = CPUMODE_EL1t;
    }

    return ctx;

    AROS_LIBFUNC_EXIT
}
