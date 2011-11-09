/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create an empty usable CPU context, x86_64 version.
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
     * Allocate common data block and FPU data block in one chunk.
     * On native ports AROSCPUContext can be simply #define'd to ExceptionContext,
     * so we refer to struct AROSCPUContext only for size calculation.
     * Below we rely on the fact that returned address is 16-byte-aligned.
     * MemChunk is two IPTRs, so this is true.
     */
    ctx = krnAllocCPUContext();
    if (ctx)
    {
    	UBYTE current_xmm[512+15];
	UBYTE *curr = (UBYTE *)AROS_ROUNDUP2((IPTR)current_xmm, 16);
	IPTR fpdata = (IPTR)ctx + AROS_ROUNDUP2(sizeof(struct AROSCPUContext), 16);

        ctx->Flags  = ECF_FPX;
	ctx->FXData = (struct FPXContext *)fpdata;

	/* Set up default values for some registers */
	ctx->rflags = 0x3202;

/* These definitions may come from machine-specific kernel_cpu.h */
#ifdef USER_CS
	ctx->Flags |= ECF_SEGMENTS;
	ctx->cs     = USER_CS;
	ctx->ds     = USER_DS;
	ctx->es     = USER_DS;
	ctx->fs     = USER_DS;
	ctx->gs     = USER_GS;
	ctx->ss     = USER_DS;
#endif

	/* Init SSE context */
	asm volatile(
	    "	fxsave (%0)\n"
	    "	fninit\n"
	    "	fwait\n"
	    "	fxsave (%1)\n"
	    "	fxrstor (%0)\n"
	    ::"r"(curr), "r"(ctx->FXData):"cc");
    }

    return ctx;

    AROS_LIBFUNC_EXIT
}
