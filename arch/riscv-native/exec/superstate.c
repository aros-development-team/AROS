/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    Desc: SuperState() - Switch the processor into a higher plane.
*/

#include <proto/exec.h>

/* See rom/exec/superstate.c for documentation */

AROS_LH0(APTR, SuperState,
    struct ExecBase *, SysBase, 25, Exec)
{
    AROS_LIBFUNC_INIT

    register unsigned int superSP = 0;

    return (APTR)superSP;

    AROS_LIBFUNC_EXIT
} /* SuperState() */
