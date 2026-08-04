/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: NewStackSwap() - Call a function with a swapped stack, 64bit RISC-V version.

    Translated from the aarch64-all version. The LP64 ABI passes up to 8
    integer args in a0-a7, so - unlike RV32 - none are pushed to the new
    stack; the inline asm loads all eight from the StackSwapArgs into
    a0-a7 before the call. The Enable()/Disable() vector offsets (-21 and
    -20 * LIB_VECTSIZE = 8) are -168 and -160; they are hardcoded here
    because the generated asm.h symbols are not visible to C inline asm
    (the arm-all and aarch64-all versions do the same).
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

    /* LP64 passes up to 8 integer args in registers, so nothing is pushed
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
        "   mv   s2, sp\n"               /* save original sp (callee-saved)  */
        "   mv   s4, %[sysbase]\n"       /* keep SysBase across entry() call */
        "   mv   s5, %[args]\n"
        "   mv   s6, %[entry]\n"
        "   mv   sp, %[newsp]\n"         /* switch to the new stack          */

        /* Enable() -- vector at SysBase + (-21 * LIB_VECTSIZE) = -168 */
        "   mv   a0, s4\n"
        "   ld   t0, -168(a0)\n"
        "   jalr t0\n"

        /* Load up to 8 register args and call the entry point */
        "   ld   a0, 0(s5)\n"
        "   ld   a1, 8(s5)\n"
        "   ld   a2, 16(s5)\n"
        "   ld   a3, 24(s5)\n"
        "   ld   a4, 32(s5)\n"
        "   ld   a5, 40(s5)\n"
        "   ld   a6, 48(s5)\n"
        "   ld   a7, 56(s5)\n"
        "   jalr s6\n"
        "   mv   s3, a0\n"               /* save the return value            */

        /* Disable() -- vector at SysBase + (-20 * LIB_VECTSIZE) = -160 */
        "   mv   a0, s4\n"
        "   ld   t0, -160(a0)\n"
        "   jalr t0\n"

        "   mv   sp, s2\n"               /* restore original sp              */
        "   mv   %[ret], s3\n"
        : [ret] "=r"(ret)
        : [entry] "r"(entry), [newsp] "r"(sp), [args] "r"(args), [sysbase] "r"(SysBase)
        : "ra","t0","t1","t2","t3","t4","t5","t6",
          "a0","a1","a2","a3","a4","a5","a6","a7",
          "s2","s3","s4","s5","s6",
          "fa0","fa1","fa2","fa3","fa4","fa5","fa6","fa7",
          "ft0","ft1","ft2","ft3","ft4","ft5","ft6","ft7",
          "ft8","ft9","ft10","ft11",
          "memory"
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
