/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.

    Desc: Create an empty usable CPU context, x86_64 version.
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

        D(bug("[Kernel] %s: Context data @ 0x%p (%u bytes)\n", __func__, ctx, KernelBase->kb_ContextSize);)

#if (AROS_FLAVOUR == AROS_FLAVOUR_STANDALONE)
        if (KernelBase->kb_ContextSize > (sizeof(struct AROSCPUContext) + sizeof(struct FPFXSContext) +  16))
        {
            ctx->FPUCtxSize = KernelBase->kb_ContextSize - (sizeof(struct AROSCPUContext) +  63);
            fpdata = AROS_ROUNDUP2((IPTR)ctx + sizeof(struct AROSCPUContext), 64);
            ctx->Flags  = ECF_FPXS;
            ctx->XSData = (struct FPXSContext *)fpdata;
            D(bug("[Kernel] %s: AVX context data @ 0x%p (size = %u)\n", __func__, ctx->XSData, ctx->FPUCtxSize);)
        }
        else
        {
            /* Legacy x86 FPU / XMM / MXCSR state */
            ctx->FPUCtxSize = sizeof(struct FPFXSContext);
#else
        {
#endif
            fpdata = AROS_ROUNDUP2((IPTR)ctx + sizeof(struct AROSCPUContext), 16);
            ctx->Flags  = ECF_FPFXS;
            ctx->FXSData = (struct FPFXSContext *)fpdata;
            D(bug("[Kernel] %s: SSE context data @ 0x%p\n", __func__, ctx->FXSData);)
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
            D(bug("[Kernel] %s: saving initial AVX state to 0x%p\n", __func__, ctx->XSData);)

            asm volatile(
                "       xor %%edx, %%edx\n"
                "       mov $0b111, %%eax\n"        /* Load instruction mask */
                "       xsave (%0)"
                ::"r"(ctx->XSData): "rax", "rdx");
        }
        else if (ctx->Flags  & ECF_FPFXS)
        {
            UBYTE current_xmm[512+15];
            UBYTE *curr = (UBYTE *)AROS_ROUNDUP2((IPTR)current_xmm, 16);

            D(bug("[Kernel] %s: saving initial SSE state\n", __func__);)

            /* Init SSE context */
            asm volatile(
                "       fxsave (%0)\n"
                "       fninit\n"
                "       fwait\n"
                "       fxsave (%1)\n"
                "       fxrstor (%0)\n"
                ::"r"(curr), "r"(ctx->FXSData):"cc");
        }
    }

    D(bug("[Kernel] %s: returning ctx = 0x%p\n", __func__, ctx);)

    return ctx;

    AROS_LIBFUNC_EXIT
}
