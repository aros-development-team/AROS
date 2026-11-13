/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    Desc: SuperState() - Switch the processor into a higher plane.
*/

#include <proto/exec.h>

#include "kernel_cpu.h"

/* See rom/exec/superstate.c for documentation */

AROS_LH0(APTR, SuperState,
    struct ExecBase *, SysBase, 25, Exec)
{
    AROS_LIBFUNC_INIT

    register uint64_t superSP;

    asm volatile (
        "       str     x30, [sp, #-16]!        \n"
        "       mov     x1, sp                  \n"
        "       svc     %[svc_no]               \n"
        "       mov     %[superSP], sp          \n"
        "       mov     sp, x1                  \n"
        "       ldr     x30, [sp], #16          \n"
        : [superSP] "=r" (superSP)
        : [svc_no] "I" (6 /*SC_SUPERSTATE*/) : "x1"
    );

    return (APTR)superSP;

    AROS_LIBFUNC_EXIT
} /* SuperState() */
