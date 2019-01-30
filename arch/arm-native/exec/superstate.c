/*
    Copyright � 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SuperState() - Switch the processor into a higher plane.
    Lang: english
*/

#include <proto/exec.h>

/* See rom/exec/superstate.c for documentation */

AROS_LH0(APTR, SuperState,
    struct ExecBase *, SysBase, 25, Exec)
{
    AROS_LIBFUNC_INIT

    register unsigned int superSP;

    asm volatile (
        "       stmfd   sp!, {lr}               \n"
        "       mov     r1, sp                  \n"
        "       swi     %[swi_no]               \n"
        "       mov     %[superSP], sp          \n"
        "       mov     sp, r1                  \n"
        "       ldmfd   sp!, {lr}               \n"
        : [superSP] "=r" (superSP)
        : [swi_no] "I" (6 /*SC_SUPERSTATE*/) : "r1"
    );

    return (APTR)superSP;

    AROS_LIBFUNC_EXIT
} /* SuperState() */
