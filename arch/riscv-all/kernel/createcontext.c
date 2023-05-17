/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    Desc: Create an empty usable CPU context, RISC-V version.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <aros/riscv/cpucontext.h>

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

    }

    return ctx;

    AROS_LIBFUNC_EXIT
}
