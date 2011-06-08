/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: NewStackSwap() - Call a function with swapped stack.
    Lang: english
*/

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <exec/tasks.h>
#include <proto/exec.h>

/*
 * The following includes need to go in this particular order!
 * We need to define _XOPEN_SOURCE (done in the first line of exec_platform.h)
 * for the proper context definition under Darwin before include any host headers,
 * otherwise ucontext_t will get only partial definition and getcontext() will clobber
 * your stack!
 */
#include "exec_intern.h"

#include <signal.h>
#include <string.h>

static void trampoline(IPTR (*func)(), IPTR *ret, IPTR *args)
{
    /* this was called from NewStackSwap() which also called Disable */
    Enable();

#if DEBUG
    int i;

    bug("[NewStackSwap] Stack swapping done\n");
    bug("[NewStackSwap] SP at 0x%p\n", AROS_GET_SP);
    bug("[NewStackSwap] Function address: 0x%p\n", func);
    for (i = 0; i < 8; i++)
    	bug("[NewStackSwap] args[%u] = 0x%p\n", i, (void *)args[i]);
#endif

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
    volatile APTR splower, spupper;
    ucontext_t ucx, ucx_return;

    D(bug("NewStackSwap(0x%p, 0x%p, 0x%p, 0x%p)\n", sss, entry, args, SysBase));
    DB2(bug("[NewStackSwap] Context size: %u\n", sizeof(ucontext_t)));

    Disable();  /* To avoid random crashes during startup */
    PD(SysBase).SysIFace->getcontext(&ucx);
    AROS_HOST_BARRIER
    Enable();

    D(bug("[NewStackSwap] getcontext() done, arguments: 0x%p, 0x%p, 0x%p, 0x%p\n", sss, entry, args, SysBase));

    /* Prepare the alternate stack */
    ucx.uc_stack.ss_sp    = sss->stk_Lower;
    ucx.uc_stack.ss_size  = (size_t)sss->stk_Pointer - (size_t)sss->stk_Lower;
    ucx.uc_stack.ss_flags = SS_ONSTACK;
    ucx.uc_link           = &ucx_return;

    if (me->tc_Flags & TF_STACKCHK)
    	memset(ucx.uc_stack.ss_sp, 0xE1, ucx.uc_stack.ss_size);

    D(bug("[NewStackSwap] Prepared stack: 0x%p - 0x%p (size %u bytes)\n", sss->stk_Lower, sss->stk_Pointer, ucx.uc_stack.ss_size));

    PD(SysBase).SysIFace->makecontext(&ucx, (void *(*)()) trampoline, 3, entry, &ret, args->Args);
    AROS_HOST_BARRIER

    DB2(bug("[NewStackSwap] Prepared context, doing stack swap\n"));

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
    AROS_HOST_BARRIER

    me->tc_SPLower = splower;
    me->tc_SPUpper = spupper;
    Enable();

    return ret;
    
    AROS_LIBFUNC_EXIT
} /* NewStackSwap() */
