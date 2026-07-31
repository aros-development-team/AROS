/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Create an empty usable CPU context, 64bit RISC-V version.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <aros/riscv64/cpucontext.h>

#include <kernel_base.h>
#include <kernel_objects.h>

/* Bytes per vector register, probed at boot (see cpu_init.c) */
extern unsigned long __riscv_vlenb;

AROS_LH0(void *, KrnCreateContext,
         struct KernelBase *, KernelBase, 18, Kernel)

{
    AROS_LIBFUNC_INIT

    struct ExceptionContext *ctx;

    /*
     * Allocate the common data block, the FPU data block and - when the
     * vector extension was detected at boot - the vector data block in
     * one chunk (sized by cpu_Init()). This way we simplify things a lot.
     *
     * On native ports AROSCPUContext can be simply #define'd to
     * ExceptionContext, so we refer struct AROSCPUContext only for size
     * calculation.
     */
    ctx = krnAllocCPUContext();
    if (ctx)
    {
        IPTR ptr = ((IPTR)ctx + sizeof(struct ExceptionContext) + 15) & ~15;

        ctx->fpuContext = (APTR)ptr;

        if (__riscv_vlenb)
        {
            struct VectorContext *vctx;

            ptr = (ptr + sizeof(struct FpuContext) + 15) & ~15;
            vctx = (struct VectorContext *)ptr;
            vctx->vlenb = __riscv_vlenb;
            ctx->vecContext = vctx;
        }
    }

    return ctx;

    AROS_LIBFUNC_EXIT
}
