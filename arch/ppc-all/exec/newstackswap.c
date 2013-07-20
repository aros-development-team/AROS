/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: NewStackSwap() - Call a function with swapped stack.
    Lang: english
*/

#define __AROS_GIMME_DEPRECATED_STACKSWAP__

#include <aros/debug.h>
#include <exec/tasks.h>
#include <proto/exec.h>

/* See rom/exec/newstackswap.c for documentation */

AROS_LH3(IPTR, NewStackSwap,
    AROS_LHA(struct StackSwapStruct *,  sss, A0),
    AROS_LHA(LONG_FUNC, entry, A1),
    AROS_LHA(struct StackSwapArgs *, args, A2),
    struct ExecBase *, SysBase, 134, Exec)
{
    AROS_LIBFUNC_INIT

    ULONG *	retptr;
    ULONG	ret;
    register ULONG real_sp asm("r1");
    ULONG *sp;
    ULONG *src;
    ULONG *dst;

    /* Get the real stack pointer */
    asm volatile ("mr %0,%1":"=r"(sp):"r"(real_sp));
        
    /* Go one stack frame upper - now src points to the stackframe of caller */
    src = (ULONG*)*sp;
        
    /* Go one more stack frame up. Now you may copy from src to dst (src - sp) IPTR's */ 
    src = (ULONG*)*src;

    dst = (ULONG*)((IPTR)sss->stk_Upper - SP_OFFSET);
    
    /* Copy the two stack frames */
    while (src != sp)
    {
        *--dst = *--src;
    }

    sss->stk_Pointer = dst;

    retptr = &ret;

    D(bug("In NewStackSwap() entry=%lx, *entry=%lx\n", (IPTR)entry, (IPTR)*entry));
    D(bug("[sss] %08x %08x %08x\n", sss->stk_Lower, sss->stk_Pointer, sss->stk_Upper));
    
    StackSwap(sss);

    D(bug("[sss] %08x %08x %08x\n", sss->stk_Lower, sss->stk_Pointer, sss->stk_Upper));

    /* Call the function with the new stack */
    *retptr = entry(args->Args[0], args->Args[1], args->Args[2], args->Args[3],
		    args->Args[4], args->Args[5], args->Args[6], args->Args[7]);

    StackSwap(sss);

    return ret;
    
    AROS_LIBFUNC_EXIT
} /* NewStackSwap() */
