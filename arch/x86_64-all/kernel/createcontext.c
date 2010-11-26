#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_memory.h>

AROS_LH0(void *, KrnCreateContext,
	  struct KernelBase *, KernelBase, 18, Kernel)

{
    AROS_LIBFUNC_INIT

    struct ExceptionContext *ctx;

    /*
     * Allocate common data block and FPU data block in one chunk.
     * On native ports AROSCPUContext can be simply #define'd to ExceptionContext,
     * so we refer struct AROSCPUContext only for size calculation.
     */
    ctx = krnAllocMem(KernelBase->kb_ContextSize);
    if (ctx)
    {
    	UBYTE current_xmm[512+15];
	UBYTE *curr = (UBYTE *)(((IPTR)current_xmm + 15) & ~15);
	IPTR fpdata = (IPTR)ctx + sizeof(struct AROSCPUContext);

	fpdata = (fpdata + 15) & ~15;
        ctx->Flags  = ECF_FPX;
	ctx->FXData = (struct FPXContext *)fpdata;

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
