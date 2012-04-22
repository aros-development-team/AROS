#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_objects.h>

AROS_LH0I(void *, KrnCreateContext,
	  struct KernelBase *, KernelBase, 18, Kernel)
{
    AROS_LIBFUNC_INIT

    struct ExceptionContext *ctx;

    /* Allocate a new context */
    ctx = krnAllocCPUContext();

    /* Initialize the context */
    if (ctx)
    {
        ULONG i;

	/*
	 * Sys V PPC ABI says r2 is reserved for TOC or SDATA2.
	 * Here we copy the current value and it will never be
	 * changed again. It is not needed for AROS but can be
	 * needed for host OS. It is known to be needed for Linux
	 * and won't harm anywhere else.
	 */
	__asm__ __volatile__ ("stw 2,%0":"=m"(ctx->gpr[2])::"memory");

	/* Initialize FPU portion */
	ctx->Flags = ECF_FPU;
        for (i = 0; i < 32; i++)
            ctx->fpr[i] = 0.0;
    }

    return ctx;

    AROS_LIBFUNC_EXIT
}
