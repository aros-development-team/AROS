/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: NewStackSwap() - Call a function with a swapped stack, AArch64 version.

    Translated from the arm-all version. On AArch64 the first 8 integer args go
    in x0-x7 (AAPCS64), so -- unlike ARM -- none are pushed to the new stack;
    the inline asm loads all eight from the StackSwapArgs into x0-x7 before the
    call. The Enable()/Disable() vector offsets (-21 and -20 * LIB_VECTSIZE = 8)
    are -168 and -160; they are hardcoded here because the generated asm.h
    symbols are not visible to C inline asm (the arm-all version does the same).
*/

#include <aros/debug.h>
#include <exec/tasks.h>
#include <proto/exec.h>

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

    /* AAPCS64 passes up to 8 integer args in registers, so nothing is pushed
       to the new stack. Just guard against a NULL args pointer. */
    if (args == NULL)
        args = (struct StackSwapArgs *)splower;   /* dummy; values are ignored */

    if (t->tc_Flags & TF_STACKCHK)
    {
        UBYTE* startfill = sss->stk_Lower;

        while (startfill < (UBYTE *)sp)
            *startfill++ = 0xE1;
    }

    /*
     * We must Disable() before changing limits and SP, otherwise a stack check
     * could fail if we are interrupted in the middle of this.
     */
    D(bug("[NewStackSwap] SP 0x%p, entry point 0x%p\n", sp, entry));
    Disable();

    /* Change limits. The actual stack switch + call is in asm below. */
    t->tc_SPReg = (APTR)sp;
    t->tc_SPLower = sss->stk_Lower;
    t->tc_SPUpper = sss->stk_Upper;

    asm volatile
    (
        "   mov  x19, sp\n"              /* save original sp (callee-saved)  */
        "   mov  x21, %[sysbase]\n"      /* keep SysBase across entry() call */
        "   mov  sp,  %[newsp]\n"        /* switch to the new stack          */

        /* Enable() -- vector at SysBase + (-21 * LIB_VECTSIZE) = -168 */
        "   mov  x0,  x21\n"
        "   ldr  x16, [x0, #-168]\n"
        "   blr  x16\n"

        /* Load up to 8 register args and call the entry point */
        "   ldp  x0, x1, [%[args], #0]\n"
        "   ldp  x2, x3, [%[args], #16]\n"
        "   ldp  x4, x5, [%[args], #32]\n"
        "   ldp  x6, x7, [%[args], #48]\n"
        "   blr  %[entry]\n"
        "   mov  x20, x0\n"              /* save the return value            */

        /* Disable() -- vector at SysBase + (-20 * LIB_VECTSIZE) = -160 */
        "   mov  x0,  x21\n"
        "   ldr  x16, [x0, #-160]\n"
        "   blr  x16\n"

        "   mov  sp,  x19\n"             /* restore original sp              */
        "   mov  %[ret], x20\n"
        : [ret] "=r"(ret)
        : [entry] "r"(entry), [newsp] "r"(sp), [args] "r"(args), [sysbase] "r"(SysBase)
        : "x0","x1","x2","x3","x4","x5","x6","x7","x16","x17",
          "x19","x20","x21","x30","cc","memory"
    );

    /* Change limits back and return */
    t->tc_SPReg = spreg;
    t->tc_SPLower = splower;
    t->tc_SPUpper = spupper;
    Enable();

    D(bug("[NewStackSwap] Returning 0x%p\n", ret));
    return ret;

    AROS_LIBFUNC_EXIT
} /* NewStackSwap() */
