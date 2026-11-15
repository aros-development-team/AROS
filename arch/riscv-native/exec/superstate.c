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
     * The kernel and every task run in the kernel's privilege mode (S-mode
     * on top of SBI firmware, where M-mode belongs to the SEE; or M-mode
     * itself) - there is no higher plane to switch to. Once user-mode task
     * separation exists this will trap into the kernel like the other
     * native ports.
     */
    return NULL;

    AROS_LIBFUNC_EXIT
} /* SuperState() */
