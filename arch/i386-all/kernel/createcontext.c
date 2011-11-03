#include <aros/kernel.h>
#include <aros/libcall.h>
#include <aros/i386/cpucontext.h>

#include <kernel_base.h>
#include <kernel_objects.h>

#ifndef SIZEOF_8087_FRAME
#define SIZEOF_8087_FRAME sizeof(struct FPUContext)
#endif

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
    ctx = krnAllocCPUContext();
    if (ctx)
    {
	IPTR fpdata = (IPTR)ctx + sizeof(struct AROSCPUContext);

        ctx->Flags  = KernelBase->kb_ContextFlags;	/* kb_ContextFlags on i386 hold only FPU bits */
	ctx->eflags = 0x3202;				/* Set up default values for some registers */

/* These definitions may come from machine-specific kernel_cpu.h */
#ifdef USER_CS
	ctx->Flags |= ECF_SEGMENTS;
	ctx->cs     = USER_CS;
	ctx->ds     = USER_DS;
	ctx->es     = USER_DS;
	ctx->fs     = USER_DS;
	ctx->gs     = USER_DS;
	ctx->ss     = USER_DS;
#endif

	if (!KernelBase->kb_ContextFlags)
	{
	    /* Don't do any of the following if we don't support FPU at all */
	    return ctx;
	}

	if (ctx->Flags & ECF_FPU)
	{
	    ctx->FPData = (struct FPUContext *)fpdata;
	    fpdata += SIZEOF_8087_FRAME;
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

	    /* fnsave implies fninit, so we don't need to do it explicitly */
	    asm volatile(
		"	fnsave (%0)\n"
		"	fnsave (%1)\n"
		"	frstor (%0)\n"
		::"r"(curr), "r"(ctx->FPData):"cc");
	}
    }

    return ctx;

    AROS_LIBFUNC_EXIT
}
