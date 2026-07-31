/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: SuperState() - Switch the processor into a higher plane.
*/

#include <proto/exec.h>

/* See rom/exec/superstate.c for documentation */

AROS_LH0(APTR, SuperState,
    struct ExecBase *, SysBase, 25, Exec)
{
    AROS_LIBFUNC_INIT

    /*
     * Everything currently runs in S-mode on top of OpenSBI - there is
     * no higher plane to switch to (M-mode belongs to the SEE). Once
     * user-mode task separation exists this will trap into the kernel
     * like the other native ports.
     */
    return NULL;

    AROS_LIBFUNC_EXIT
} /* SuperState() */
