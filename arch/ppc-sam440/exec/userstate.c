/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: UserState() - Return to normal mode after changing things.
    Lang: english
*/

#include <proto/exec.h>
#include <asm/amcc440.h>

/* See rom/exec/userstate.c for documentation */

AROS_LH1(void, UserState,
    AROS_LHA(APTR, sysStack, D0),
    struct ExecBase *, SysBase, 26, Exec)
{
    AROS_LIBFUNC_INIT

    wrmsr(rdmsr() | (MSR_PR));

    sysStack = 0;   /* Get rid of the compiler warning */

    AROS_LIBFUNC_EXIT
} /* UserState() */
