/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    Desc: NewStackSwap() - Call a function with swapped stack, x86-64 version
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
    volatile IPTR *newsp = sss->stk_Pointer;
    volatile APTR sporig = t->tc_SPReg;
    volatile APTR splower = t->tc_SPLower;
    volatile APTR spupper = t->tc_SPUpper;
    IPTR ret;

    D(
        bug("[%s] Initial stack -:\n", __func__);
        bug("[%s]      upper   0x%p\n", __func__, t->tc_SPUpper);
        bug("[%s]      current 0x%p\n", __func__, t->tc_SPReg);
        bug("[%s]      lower   0x%p\n", __func__, t->tc_SPLower);
    )
    
    if (((IPTR)newsp & (AROS_STACKALIGN - 1)) != 0) {
       bug("[%s] called with missaligned SP 0x%p (%02x)\n", __func__, newsp, (IPTR)newsp & (AROS_STACKALIGN - 1));
       newsp = (IPTR *)((IPTR)newsp & ~(AROS_STACKALIGN - 1));
        if ((IPTR)newsp <= (IPTR)splower)
        {
            bug("[%s] no space to align stack\n", __func__);
            bug("[%s] called function may cause AROS to crash,\n", __func__);
            bug("[%s] or behave incorrectly\n", __func__);
        }
    }

    if (args != NULL)
    {
        /* Only last two arguments are put to stack in x86-64 */
        if ((IPTR)newsp <= (IPTR)sss->stk_Lower + (sizeof(IPTR) << 1))
        {
            bug("[%s] no space to store function params\n", __func__);
            bug("[%s] called function may cause AROS to crash,\n", __func__);
            bug("[%s] or behave incorrectly\n", __func__);
        }
        _PUSH(newsp, args->Args[7]);
        _PUSH(newsp, args->Args[6]);
    }
    else
    {
        /* Dummy args to be put in registers below */
        args = splower;
    }

    if (t->tc_Flags & TF_STACKCHK)
    {
        UBYTE* startfill = sss->stk_Lower;

        while (startfill < (UBYTE *)newsp)
            *startfill++ = 0xE1;
    }

    /*
     * We need to Disable() before changing limits and SP, otherwise
     * stack check will fail if we get interrupted in the middle of this
     */
    D(bug("[%s] SP 0x%p, entry point 0x%p\n", __func__, newsp, entry));
    Disable();

    /* Change limits. The rest is done in asm below */
    t->tc_SPReg = (APTR)newsp;
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
    : "r"(entry), "r"(newsp), "r"(args), "D"(SysBase)
    : "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11", "cc");

    /* Change limits back and return */
    t->tc_SPReg = sporig;
    t->tc_SPLower = splower;
    t->tc_SPUpper = spupper;
    Enable();

    D(
        bug("[%s] Final stack -:\n", __func__);
        bug("[%s]      upper   0x%p\n", __func__, t->tc_SPUpper);
        bug("[%s]      current 0x%p\n", __func__, t->tc_SPReg);
        bug("[%s]      lower   0x%p\n", __func__, t->tc_SPLower);
    )

    D(bug("[%s] Returning 0x%p\n", __func__, ret));

    return ret;

    AROS_LIBFUNC_EXIT
} /* NewStackSwap() */
