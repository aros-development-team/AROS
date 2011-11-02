#include <aros/kernel.h>
#include <aros/libcall.h>
#include <aros/i386/cpucontext.h>

#include <kernel_base.h>
#include <kernel_memory.h>

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
    ctx = AllocMem(KernelBase->kb_ContextSize, MEMF_PUBLIC|MEMF_CLEAR);
    if (ctx)
    {
	IPTR fpdata = (IPTR)ctx + sizeof(struct AROSCPUContext);

        ctx->Flags = KernelBase->kb_ContextFlags;	
	/* Set up default values for some registers */
	ctx->eflags = 0x3202;

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

	if (!ctx->Flags)
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
