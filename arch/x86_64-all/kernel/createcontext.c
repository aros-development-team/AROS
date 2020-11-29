/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create an empty usable CPU context, x86_64 version.
    Lang: english
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_objects.h>
#include "kernel_debug.h"

#define D(x)

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
	IPTR fpdata;
        if (KernelBase->kb_ContextSize > AROS_ROUNDUP2(sizeof(struct AROSCPUContext), 16) + sizeof(struct FPFXSContext))
        {
            /* AVX state
             * NB: we dont need to know its actual size,
             * just that the header is 128+64bytes bigger than the legacy context
             */
            ctx->FPUCtxSize = sizeof(struct FPFXSContext) + (128 + 64);
            fpdata = AROS_ROUNDUP2((IPTR)ctx + sizeof(struct AROSCPUContext), 64);
            D(bug("[Kernel] %s: AVX context data @ 0x%p\n", __func__, fpdata);)
            ctx->Flags  = ECF_FPXS;
            ctx->XSData = (struct FPXSContext *)fpdata;
        }
        else
        {
            /* Legacy x86 FPU / XMM / MXCSR state */
            ctx->FPUCtxSize = sizeof(struct FPFXSContext);
            fpdata = AROS_ROUNDUP2((IPTR)ctx + sizeof(struct AROSCPUContext), 16);
            D(bug("[Kernel] %s: SSE context data @ 0x%p\n", __func__, fpdata);)
            ctx->Flags  = ECF_FPFXS;
            ctx->FXSData = (struct FPFXSContext *)fpdata;
        }

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

        if (ctx->Flags  & ECF_FPXS)
        {
            D(bug("[Kernel] %s: saving initial AVX state\n", __func__);)

            asm volatile("xsave (%0)"::"r"(ctx->XSData));
        }
        else if (ctx->Flags  & ECF_FPFXS)
        {
            UBYTE current_xmm[512+15];
            UBYTE *curr = (UBYTE *)AROS_ROUNDUP2((IPTR)current_xmm, 16);

            D(bug("[Kernel] %s: saving initial SSE state\n", __func__);)

            /* Init SSE context */
            asm volatile(
                "	fxsave (%0)\n"
                "	fninit\n"
                "	fwait\n"
                "	fxsave (%1)\n"
                "	fxrstor (%0)\n"
                ::"r"(curr), "r"(ctx->FXSData):"cc");
        }
    }

    D(bug("[Kernel] %s: returning ctx = 0x%p\n", __func__, ctx);)

    return ctx;

    AROS_LIBFUNC_EXIT
}
