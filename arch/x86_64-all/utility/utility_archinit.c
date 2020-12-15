#ifdef __AVX__
/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/utility.h>

extern void AROS_SLIB_ENTRY(SetMem_AVX, Utility, 66)();

static int Utilityx8664_ArchInit(struct Library *UtilityBase)
{
    struct Task *me = FindTask(NULL);
    struct ExceptionContext *ctx = me->tc_UnionETask.tc_ETask->et_RegFrame;
    BOOL setSet = FALSE;

    if (!setSet && (ctx->Flags & ECF_FPXS))
    {
        D(bug("[Utility:x86_64] Using AVX SetMem\n"));
        SetFunction(UtilityBase, -66*LIB_VECTSIZE, AROS_SLIB_ENTRY(SetMem_AVX, Utility, 66));
        setSet = TRUE;
    }

    return TRUE;
}

ADD2INITLIB(Utilityx8664_ArchInit, 0);
#endif
