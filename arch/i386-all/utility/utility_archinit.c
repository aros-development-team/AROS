#ifdef __SSE__
#define _SETMEMPATCH
#endif
#ifdef __AVX__
#define _SETMEMPATCH
#endif

#if defined(_SETMEMPATCH)
/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/utility.h>

#ifdef __AVX_
extern void AROS_SLIB_ENTRY(SetMem_AVX, Utility, 66)();
#endif
#ifdef __SSE_
extern void AROS_SLIB_ENTRY(SetMem_SSE, Utility, 66)();
#endif

static int UtilityI386_ArchInit(struct Library *UtilityBase)
{
    struct Task *me = FindTask(NULL);
    struct ExceptionContext *ctx = me->tc_UnionETask.tc_ETask->et_RegFrame;
    BOOL setSet = FALSE;

#ifdef __AVX__
    if (!setSet && (ctx->Flags & ECF_FPXS))
    {
        D(bug("[Utility:i386] Using AVX SetMem\n"));
        SetFunction(UtilityBase, -66*LIB_VECTSIZE, AROS_SLIB_ENTRY(SetMem_AVX, Utility, 66));
        setSet = TRUE;
    }
#endif
#ifdef __SSE__
    if (!setSet && (ctx->Flags & ECF_FPFXS))
    {
        D(bug("[Utility:i386] Using SSE SetMem\n"));

        /* Use SSE version of SetMem() */
        SetFunction(UtilityBase, -66*LIB_VECTSIZE, AROS_SLIB_ENTRY(SetMem_SSE, Utility, 66));
        setSet = TRUE;
    }
#endif
    return TRUE;
}

ADD2INITLIB(UtilityI386_ArchInit, 0);
#endif
