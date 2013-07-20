/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SuperState() - Switch the processor into a higher plane.
    Lang: english
*/

#include <proto/exec.h>
#include <asm/amcc440.h>

/* See rom/exec/superstate.c for documentation */

AROS_LH0(APTR, SuperState,
    struct ExecBase *, SysBase, 25, Exec)
{
    AROS_LIBFUNC_INIT

    asm volatile("li %%r3,%0; sc"::"i"(6 /*SC_SUPERSTATE*/):"memory","r3");

    /* We have to return something. NULL is an invalid address for a
       stack, so it could be used to say that this function does
       nothing.
    */
    return NULL;

    AROS_LIBFUNC_EXIT
} /* SuperState() */
