/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: NewStackSwap() - Call a function with swapped stack.
    Lang: english
*/

#include <aros/config.h>
#include <aros/debug.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#define _PUSH(sp, val) *--sp = (IPTR)val

AROS_LH3(IPTR, NewStackSwap,
	AROS_LHA(struct StackSwapStruct *,  sss, A0),
	AROS_LHA(LONG_FUNC, entry, A1),
	AROS_LHA(struct StackSwapArgs *, args, A2),
	struct ExecBase *, SysBase, 134, Exec)
{
    AROS_LIBFUNC_INIT

    volatile struct Task *t = FindTask(NULL);
    volatile IPTR *sp = sss->stk_Pointer;
    volatile APTR splower = t->tc_SPLower;
    volatile APTR spupper = t->tc_SPUpper;
    IPTR ret;
    BYTE i;

    /* Put arguments on stack in appropriate order */
    for (i = 7; i >= 0; i--)
    {
	D(bug("[NewStackSwap] Argument %d value 0x%08lX\n", i, args->Args[i]));
	_PUSH(sp, args->Args[i]);
    }

    if (t->tc_Flags & TF_STACKCHK)
    {
    	UBYTE* startfill = sss->stk_Lower;

    	while (startfill < (UBYTE *)sp)
	    *startfill++ = 0xE1;
    }

    /*
     * We need to Disable() before changing limits and SP, otherwise
     * stack check will fail if we get interrupted in the middle of this
     */
    D(bug("[NewStackSwap] SP 0x%p, entry point 0x%p\n", sp, entry));
    Disable();

    /* Change limits. The rest is done in asm below */
    t->tc_SPLower = sss->stk_Lower;
    t->tc_SPUpper = sss->stk_Upper;

    asm volatile
    (
    /* Save original ESP by setting up a new stack frame */
    "	push	%%ebp\n"
    "	movl	%%esp, %%ebp\n"
    /* Actually change the stack */
    "	movl	%2, %%esp\n\t"

    /* Enable(). It preserves all registers by convention, so no %1 save/restore. */
    "	push	%3\n"
    "	call	*-84(%3)\n"
    "	pop	%3\n"

    /* Call our function */
    "	call	*%1\n"

    /*
     * Disable(). Also preserves registers.
     * We reload %3 from global SysBase here because we are running on a new stack,
     * and local SysBase of this function is placed on old one. %3 was
     * clobbered by the called function.
     */
    "	movl	SysBase, %3\n"
    "	push	%3\n"
    "	call	*-80(%3)\n"
    "	pop	%3\n"

    /* Restore original ESP. Function's return value is in EAX. */
    "	movl	%%ebp, %%esp\n"
    "	pop	%%ebp\n"
    : "=a"(ret)
    : "r"(entry), "r"(sp), "r"(SysBase)
    : "ebx", "ecx", "edx", "cc");

    /* Change limits back and return */
    t->tc_SPLower = splower;
    t->tc_SPUpper = spupper;
    Enable();

    D(bug("[NewStackSwap] Returning 0x%08lX\n", ret));
    return ret;

    AROS_LIBFUNC_EXIT
} /* NewStackSwap() */
