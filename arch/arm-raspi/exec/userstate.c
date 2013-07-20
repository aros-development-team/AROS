/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: UserState() - Return to normal mode after changing things.
    Lang: english
*/

#include <proto/exec.h>

/* See rom/exec/userstate.c for documentation */

AROS_LH1(void, UserState,
    AROS_LHA(APTR, superSP, D0),
    struct ExecBase *, SysBase, 26, Exec)
{
    AROS_LIBFUNC_INIT

    if (superSP)
    {
        asm volatile (
            "       stmfd   sp!, {lr}               \n"
            "       mov     r1, sp                  \n"
            "       mov     sp, %[superSP]          \n"
            "       cpsie   i, %[mode_user]         \n"
            "       mov     sp, r1                  \n"
            "       ldmfd   sp!, {lr}               \n"
            : : [superSP] "r" (superSP), [mode_user] "I" (CPUMODE_USER) : "r1" );
    }
    AROS_LIBFUNC_EXIT
} /* UserState() */
