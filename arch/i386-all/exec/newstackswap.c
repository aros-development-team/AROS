/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
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
    volatile APTR spreg = t->tc_SPReg;
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
    	volatile UBYTE* startfill = sss->stk_Lower;

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
    t->tc_SPReg = (APTR)sp;
    t->tc_SPLower = sss->stk_Lower;
    t->tc_SPUpper = sss->stk_Upper;

    asm volatile
    (
    /* Save original ESP by setting up a new stack frame */
    "	push	%%ebp\n"
    "	movl	%%esp, %%ebp\n"
    /* Actually change the stack */
    "	movl	%2, %%esp\n\t"

    /* Enable(). Pass SysBase in %eax, We don't need %eax afterwards */
    "	call	*-84(%0)\n"

    /* Call our function */
    "	call	*%1\n"

    /*
     * Disable().
     * Remember %eax (e.g. %0) and put local SysBase of this function in it.
     * %3 was clobbered by the called function.
     */ 
    "	push	%0\n"
    "	movl	SysBase, %0\n"
    "	call	*-80(%0)\n"
    "	pop	%0\n"

    /* Restore original ESP. Function's return value is in EAX. */
    "	movl	%%ebp, %%esp\n"
    "	pop	%%ebp\n"
    : "=a"(ret)
    : "r"(entry), "r"(sp), "a"(SysBase)
    : "ecx", "edx", "cc");

    /* Change limits back and return */
    t->tc_SPReg = spreg;
    t->tc_SPLower = splower;
    t->tc_SPUpper = spupper;
    Enable();

    D(bug("[NewStackSwap] Returning 0x%08lX\n", ret));
    return ret;

    AROS_LIBFUNC_EXIT
} /* NewStackSwap() */
