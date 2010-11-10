#include <aros/kernel.h>
#include <aros/libcall.h>
#include <aros/arm/cpucontext.h>

#include <kernel_base.h>
#include <kernel_cpu.h>
#include <kernel_memory.h>

AROS_LH0I(void *, KrnCreateContext,
	  struct KernelBase *, KernelBase, 18, Kernel)

{
    AROS_LIBFUNC_INIT

    struct AROSCPUContext *ctx;
    ULONG fpu_size;

    /*
     * Allocate common data block and FPU data block in one
     * chunk. This way we may use FreeVec()-alike operation
     * in order to free the whole context.
     *
     * TODO: Currently we support only VFP and assume
     * it is always present. In future we need to check
     * available VPU type and select appropriate context type
     * here.
     */
    fpu_size = sizeof(struct VFPContext);

    ctx = krnAllocMem(sizeof(struct AROSCPUContext) + fpu_size);
    if (ctx)
    {
	ctx->fpuContext = &ctx[1];
	/* TODO: initialize FPU context ? */
    }

    return ctx;

    AROS_LIBFUNC_EXIT
}
