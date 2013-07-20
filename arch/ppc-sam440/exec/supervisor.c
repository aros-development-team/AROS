/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Supervisor() - Execute some code in a privileged environment.
    Lang: english
*/

#include <proto/exec.h>
#include <asm/amcc440.h>

/* See rom/exec/supervisor.c for documentation */

AROS_LH1(ULONG, Supervisor,
    AROS_LHA(ULONG_FUNC, userFunction, A5),
    struct ExecBase *, SysBase, 5, Exec)
{
    AROS_LIBFUNC_INIT

    register ULONG retval;
    register APTR stack;

    stack = SuperState();

    retval = (*userFunction)();

    UserState(stack);

    return retval;

    AROS_LIBFUNC_EXIT
} /* Supervisor() */
