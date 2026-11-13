/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: CacheClearE() - Clear the caches with extended control (AArch64).
*/

#include <aros/config.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

#include "kernel_syscall.h"

#include <proto/exec.h>

/* See rom/exec/cachecleare.c for documentation */

AROS_LH3(void, CacheClearE,
    AROS_LHA(APTR, address, A0),
    AROS_LHA(ULONG, length, D0),
    AROS_LHA(ULONG, caches, D1),
    struct ExecBase *, SysBase, 107, Exec)
{
    AROS_LIBFUNC_INIT

    register APTR addr asm("x0") = address;
    register ULONG len asm("x1") = length;
    register ULONG c asm("x2") = caches;

    asm volatile("svc %0\n\t"::"I"(SC_CACHECLEARE), "r"(addr), "r"(len), "r"(c):"memory","x30");

    AROS_LIBFUNC_EXIT
} /* CacheClearE */
