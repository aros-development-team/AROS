/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: newstackswap.c 41077 2011-09-04 16:51:40Z verhaegs $

    Desc: NewStackSwap() - Call a function with swapped stack, x86-64 version
    Lang: english
*/

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

    /* Only last four arguments are put to stack in ARM */
    _PUSH(sp, args->Args[7]);
    _PUSH(sp, args->Args[6]);
    _PUSH(sp, args->Args[5]);
    _PUSH(sp, args->Args[4]);

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
    /* Save original SP by adding one more stack frame */
    "	push	{fp, lr}\n"
    "	add	fp, sp, #4\n"
    /* Actually change the stack */
    "	mov 	sp, %2\n"

    /* Enable(). It preserves all registers by convention. */
    "	mov	r0, %4\n"
    "	ldr	r12, [r0, #-84]\n"
    "	blx	r12\n"

    /* Call our function with its arguments */
    "	ldr	r0, [%3, #0]\n"
    "	ldr	r1, [%3, #4]\n"
    "	ldr	r2, [%3, #8]\n"
    "	ldr	r3, [%3, #12]\n"
    "	blx	%1\n"

    /* Disable(). r0 is first argument, so save it. */
    "	push	{r0}\n"
    "	ldr	r0, _sysbase\n"
    "	ldr	r0, [r0]\n"
    "	ldr	r12, [r0, #-80]\n"
    "	blx	r12\n"
    "	pop	{%0}\n"

    /* Restore original SP. Function's return value is in %0 now. */
    "	sub	sp, fp, #4\n"
    "	pop	{fp, lr}\n"
    : "=r"(ret)
    : "r"(entry), "r"(sp), "r"(args), "r"(SysBase)
    : "r0", "r1", "r2", "r3", "r12", "cc");

    /* Change limits back and return */
    t->tc_SPLower = splower;
    t->tc_SPUpper = spupper;
    Enable();

    D(bug("[NewStackSwap] Returning 0x%p\n", ret));
    return ret;

    AROS_LIBFUNC_EXIT
} /* NewStackSwap() */

/* Reference to a global SysBase */
asm ("_sysbase: .word SysBase");
