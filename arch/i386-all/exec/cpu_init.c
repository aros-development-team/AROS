#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>

#include "etask.h"

extern void Exec_CopyMem_SSE();

static int cpu_Init(struct ExecBase *SysBase)
{
    /*
     * Here we want to check what features are supported by our CPU.
     * The idea is simple: we check flags of own CPU context. If kernel.resource
     * detects SSE, it allocates additional storage for SSE contexts and sets ECF_FPX
     * flag in the context structure.
     * We do this because of two reasons:
     * 1. processor.resource is not up yet.
     * 2. We are exec, and we know what to do with our own internal structures. ;-)
     * Normal applications should use processor.resource instead.
     */
    struct Task *me = FindTask(NULL);
    struct ExceptionContext *ctx = GetIntETask(me)->iet_Context;

    if (ctx->Flags & ECF_FPX)
    {
        D(bug("[exec] SSE detected\n"));

	/* Use SSE version of CopyMem() and CopyMemQuick() */
	SetFunction(&SysBase->LibNode, -104*LIB_VECTSIZE, AROS_SLIB_ENTRY(CopyMem_SSE, Exec));
	SetFunction(&SysBase->LibNode, -105*LIB_VECTSIZE, AROS_SLIB_ENTRY(CopyMem_SSE, Exec));
    }

    return TRUE;
}

ADD2INITLIB(cpu_Init, 0);
