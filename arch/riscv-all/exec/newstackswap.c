/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    Desc: NewStackSwap() - Call a function with swapped stack, RISC-V version
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
    volatile APTR spreg = t->tc_SPReg;
    volatile APTR splower = t->tc_SPLower;
    volatile APTR spupper = t->tc_SPUpper;
    IPTR ret;

    if (args != NULL)
    {
        /* Push necessary arguments to stack in RISC-V */
        _PUSH(sp, args->Args[7]);
        _PUSH(sp, args->Args[6]);
#if (RISCV_FUNCREG_CNT <5)
        _PUSH(sp, args->Args[5]);
#endif
#if (RISCV_FUNCREG_CNT <4)
        _PUSH(sp, args->Args[4]);
#endif
#if (RISCV_FUNCREG_CNT <3)
        _PUSH(sp, args->Args[3]);
#endif
#if (RISCV_FUNCREG_CNT <2)
        _PUSH(sp, args->Args[2]);
#endif
#if (RISCV_FUNCREG_CNT <1)
        _PUSH(sp, args->Args[1]);
#endif
#if (RISCV_FUNCREG_CNT == 0)
        _PUSH(sp, args->Args[0]);
#endif
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



    /* Change limits back and return */
    t->tc_SPReg = spreg;
    t->tc_SPLower = splower;
    t->tc_SPUpper = spupper;
    Enable();

    D(bug("[NewStackSwap] Returning 0x%p\n", ret));
    return ret;

    AROS_LIBFUNC_EXIT
} /* NewStackSwap() */

/* Reference to a global SysBase */
asm ("_sysbase: .word SysBase");
