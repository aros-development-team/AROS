/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>

#include <defines/exec_LVO.h>

extern void AROS_SLIB_ENTRY(CopyMem_SSE, Exec, LVOCopyMem)();
extern void AROS_SLIB_ENTRY(CopyMemQuick_SSE, Exec, LVOCopyMemQuick)();

static int cpu_Init(struct ExecBase *SysBase)
{
    /*
     * Here we want to check what features are supported by our CPU.
     * The idea is simple: we check flags of own CPU context. If kernel.resource
     * detects SSE, it allocates additional storage for SSE contexts and sets ECF_FPFXS
     * flag in the context structure.
     * We do this because of two reasons:
     * 1. processor.resource is not up yet.
     * 2. We are exec, and we know what to do with our own internal structures. ;-)
     * Normal applications should use processor.resource instead.
     */
    struct Task *me = FindTask(NULL);
    struct ExceptionContext *ctx = me->tc_UnionETask.tc_ETask->et_RegFrame;

    if (ctx->Flags & ECF_FPFXS)
    {
        D(bug("[Exec] SSE detected\n"));

        /* Use SSE version of CopyMem() and CopyMemQuick() */
        SetFunction(&SysBase->LibNode, -LVOCopyMem*LIB_VECTSIZE, AROS_SLIB_ENTRY(CopyMem_SSE, Exec, LVOCopyMem));
        SetFunction(&SysBase->LibNode, -LVOCopyMemQuick*LIB_VECTSIZE, AROS_SLIB_ENTRY(CopyMemQuick_SSE, Exec, LVOCopyMemQuick));
    }

    return TRUE;
}

ADD2INITLIB(cpu_Init, 0);
