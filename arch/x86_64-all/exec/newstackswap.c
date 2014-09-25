/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: NewStackSwap() - Call a function with swapped stack, x86-64 version
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

    if (args != NULL)
    {
        /* Only last two arguments are put to stack in x86-64 */
        _PUSH(sp, args->Args[7]);
        _PUSH(sp, args->Args[6]);
    }
    else
    {
        /* Dummy args to be put in registers below */
        args = splower;
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
    t->tc_SPReg = (APTR)sp;
    t->tc_SPLower = sss->stk_Lower;
    t->tc_SPUpper = sss->stk_Upper;

    asm volatile(
    /* Save original RSP */
    "push %%rbp\n\t"
    "movq %%rsp, %%rbp\n\t"
    /* Actually change the stack */
    "movq %2, %%rsp\n\t"

    /* Enable(). It preserves all registers by convention. */
    "call *-168(%%rdi)\n\t"

    /* Call our function with its arguments */
    "movq 0(%3), %%rdi\n\t"
    "movq 8(%3), %%rsi\n\t"
    "movq 16(%3), %%rdx\n\t"
    "movq 24(%3), %%rcx\n\t"
    "movq 32(%3), %%r8\n\t"
    "movq 40(%3), %%r9\n\t"
    "call *%1\n\t"

    /* Disable(). Also preserves registers. */
    "movabsq $SysBase, %%rdi\n\t"
    "movq (%%rdi), %%rdi\n\t"
    "call *-160(%%rdi)\n\t"

    /* Restore original RSP. Function's return value is in RAX. */
    "movq %%rbp, %%rsp\n\t"
    "popq %%rbp\n"
    : "=a"(ret)
    : "r"(entry), "r"(sp), "r"(args), "D"(SysBase)
    : "rsi", "rdx", "rcx", "r8", "r9", "cc");

    /* Change limits back and return */
    t->tc_SPReg = spreg;
    t->tc_SPLower = splower;
    t->tc_SPUpper = spupper;
    Enable();

    D(bug("[NewStackSwap] Returning 0x%p\n", ret));
    return ret;

    AROS_LIBFUNC_EXIT
} /* NewStackSwap() */
