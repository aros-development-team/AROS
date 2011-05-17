#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_memory.h>

AROS_LH0I(void *, KrnCreateContext,
	  struct KernelBase *, KernelBase, 18, Kernel)
{
    AROS_LIBFUNC_INIT

    struct ExceptionContext *ctx;
    cpumode_t mode = goSuper();

    /* Our context is not accessible in user mode */
    ctx = krnAllocMem(sizeof(struct AROSCPUContext), 0);

    /* Initialize the context */
    if (ctx)
    {
        ULONG i;

	/*
	 * Sys V PPC ABI says r2 is reserved so we copy the
	 * current one and it will never be changed again
	 */
	__asm__ __volatile__ ("stw 2,%0":"=m"(ctx->gpr[2])::"memory");

	/* Initialize FPU portion */
	ctx->Flags = ECF_FPU;
        for (i = 0; i < 32; i++)
            ctx->fpr[i] = 0.0;
    }

    goBack(mode);

    return ctx;

    AROS_LIBFUNC_EXIT
}
