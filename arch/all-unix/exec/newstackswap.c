/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: NewStackSwap() - Call a function with swapped stack.
    Lang: english
*/

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#include <signal.h>

#include "exec_intern.h"

static void trampoline(IPTR (*func)(), IPTR *ret, IPTR *args)
{
    /* this was called from NewStackSwap() which also called Disable */
    Enable();
    
    *ret = func(args[0], args[1], args[2], args[3],
		args[4], args[5], args[6], args[7]);

    /* this was called from NewStackSwap() which will enable again */        
    Disable();
}

AROS_LH3(IPTR, NewStackSwap,
	 AROS_LHA(struct StackSwapStruct *,  sss, A0),
	 AROS_LHA(LONG_FUNC, entry, A1),
	 AROS_LHA(struct StackSwapArgs *, args, A2),
	 struct ExecBase *, SysBase, 122, Exec)
{
    AROS_LIBFUNC_INIT

    struct Task *me = FindTask(NULL);
    IPTR ret;
    APTR splower, spupper;
    ucontext_t ucx, ucx_return;

    Disable();  /* To avoid random crashes during startup */
    PD(SysBase).SysIFace->getcontext(&ucx);
    Enable();

    /* Prepare the alternate stack */
    ucx.uc_stack.ss_sp    = sss->stk_Lower;
    ucx.uc_stack.ss_size  = (size_t)sss->stk_Pointer - (size_t)sss->stk_Lower;
    ucx.uc_stack.ss_flags = SS_ONSTACK;
    ucx.uc_link           = &ucx_return;

    PD(SysBase).SysIFace->makecontext(&ucx, (void *(*)()) trampoline, 3, entry, &ret, args->Args);

    /* Remember original stack limits */
    splower = me->tc_SPLower;
    spupper = me->tc_SPUpper;

    /* Disable(), otherwise stack check will fail */
    Disable();

    /* Set new stack limits, swapcontext() will change the stack itself */
    me->tc_SPLower = sss->stk_Lower;
    me->tc_SPUpper = sss->stk_Upper;

    /* We Enable() in trampoline */
    PD(SysBase).SysIFace->swapcontext(&ucx_return, &ucx);

    me->tc_SPLower = splower;
    me->tc_SPUpper = spupper;
    Enable();

    return ret;
    
    AROS_LIBFUNC_EXIT
} /* NewStackSwap() */
