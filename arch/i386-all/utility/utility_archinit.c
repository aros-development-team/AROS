/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/utility.h>

extern void AROS_SLIB_ENTRY(SetMem_SSE, Utility, 66)();

static int utility_archinit(struct Library *UtilityBase)
{
    struct Task *me = FindTask(NULL);
    struct ExceptionContext *ctx = me->tc_UnionETask.tc_ETask->et_RegFrame;

    if (ctx->Flags & ECF_FPX)
    {
        D(bug("[utility] SSE detected\n"));

        /* Use SSE version of SetMem() */
        SetFunction(UtilityBase, -66*LIB_VECTSIZE, AROS_SLIB_ENTRY(SetMem_SSE, Utility, 66));
    }

    return TRUE;
}

ADD2INITLIB(utility_archinit, 0);
