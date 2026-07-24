/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    Desc: UserState() - Return to normal mode after changing things.
*/

#include <proto/exec.h>

#include "kernel_cpu.h"
#include "kernel_syscall.h"

/* See rom/exec/userstate.c for documentation */

AROS_LH1(void, UserState,
    AROS_LHA(APTR, superSP, D0),
    struct ExecBase *, SysBase, 26, Exec)
{
    AROS_LIBFUNC_INIT

    if (superSP)
    {
        /*
         * Counterpart of SuperState(): we are running at EL1h on the task's
         * own stack (SuperState moved SP_EL1 there). Restore SP_EL1 to the
         * exception stack value SuperState() returned, then SVC to have the
         * kernel flip the saved SPSR back to EL1t with IRQs enabled. The
         * ERET switches us to SP_EL0, which is stale (it holds the sp from
         * the SuperState call), so restore the live task sp from x1 after
         * the SVC before popping x30.
         */
        asm volatile (
            "       str     x30, [sp, #-16]!        \n"
            "       mov     x1, sp                  \n"
            "       mov     sp, %[superSP]          \n"
            "       svc     %[svc_no]               \n"
            "       mov     sp, x1                  \n"
            "       ldr     x30, [sp], #16          \n"
            : : [superSP] "r" (superSP), [svc_no] "I" (SC_USERSTATE) : "x1" );
    }
    AROS_LIBFUNC_EXIT
} /* UserState() */
