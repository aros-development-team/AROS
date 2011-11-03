/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create an empty usable CPU context, ARM version.
    Lang: english
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_objects.h>

AROS_LH0(void *, KrnCreateContext,
	 struct KernelBase *, KernelBase, 18, Kernel)

{
    AROS_LIBFUNC_INIT

    struct ExceptionContext *ctx;

    /*
     * Allocate common data block and FPU data block in one
     * chunk. This way we simplify things a lot.
     * 
     * On native ports AROSCPUContext can be simply #define'd to ExceptionContext,
     * so we refer struct AROSCPUContext only for size calculation.
     */
    ctx = krnAllocCPUContext();
    if (ctx)
    {
        ctx->FPUType    = KernelBase->kb_ContextFlags;
        ctx->cpsr       = 0x10;		/* Initial value for user mode */
	ctx->fpuContext = (APTR)((IPTR)ctx + sizeof(struct AROSCPUContext));
    }

    return ctx;

    AROS_LIBFUNC_EXIT
}
