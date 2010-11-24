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
	IPTR fpdata = (IPTR)ctx + sizeof(struct AROSCPUContext);

        ctx->Flags = KernelBase->kb_ContextFlags;
	if (!ctx->Flags)
	    /* Don't do any of the following if we don't support FPU at all */
	    return ctx;

	if (ctx->Flags & ECF_FPU)
	{
	    ctx->FPData = (struct FPUContext *)fpdata;
	    fpdata += 112;
	}

	if (ctx->Flags & ECF_FPX)
	{
	    UBYTE current_xmm[512+15];
	    UBYTE *curr = (UBYTE *)(((IPTR)current_xmm + 15) & ~15);

	    fpdata = (fpdata + 15) & ~15;
	    ctx->FXData = (struct FPXContext *)fpdata;

	    asm volatile(
		"	fxsave (%0)\n"
		"	fninit\n"
		"	fwait\n"
		"	fxsave (%1)\n"
		"	andl %2, %2\n"
		"	je 1f\n"
		"	fnsave (%2)\n"
		"1:	fxrstor (%0)\n"
		::"r"(curr), "r"(ctx->FXData), "r"(ctx->FPData):"cc");
	}
	else if (ctx->Flags & ECF_FPU)
	{
	    UBYTE curr[112];

	    asm volatile(
		"	fnsave (%0)\n"
		"	fninit\n"
		"	fwait\n"
		"	fnsave (%1)\n"
		"	frstor (%0)\n"
		::"r"(curr), "r"(ctx->FPData):"cc");
	}
    }

    return ctx;

    AROS_LIBFUNC_EXIT
}
