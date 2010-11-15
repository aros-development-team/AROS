#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_memory.h>

AROS_LH0I(void *, KrnCreateContext,
	  struct KernelBase *, KernelBase, 18, Kernel)

{
    AROS_LIBFUNC_INIT

    struct ExceptionContext *ctx;
    UBYTE fpu_type;
    ULONG fpu_size;

    /*
     * Allocate common data block and FPU data block in one
     * chunk. This way we may use FreeVec()-alike operation
     * in order to free the whole context.
     * TODO: in future this can be extended to support more than
     * a single FPU type.
     */
    fpu_type = ARM_FPU_TYPE;
    fpu_size = ARM_FPU_SIZE;

    /*
     * On native ports AROSCPUContext can be simply #define'd to ExceptionContext,
     * so we refer struct AROSCPUContext only for size calculation.
     */
    ctx = krnAllocVec(sizeof(struct AROSCPUContext) + fpu_size);
    if (ctx)
    {
        ctx->FPUType    = fpu_type;
        ctx->cpsr       = 0x10;		/* Initial value for user mode */
	ctx->fpuContext = (APTR)((IPTR)ctx + sizeof(struct AROSCPUContext));
	/* TODO: initialize FPU context ? */
    }

    return ctx;

    AROS_LIBFUNC_EXIT
}
