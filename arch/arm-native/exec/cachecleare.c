/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CacheClearE() - Clear the caches with extended control.
    Lang: english
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

    register APTR addr asm("r0") = address;
    register ULONG len asm("r1") = length;
    register ULONG c asm ("r2") = caches;

    asm volatile("swi %0\n\t"::"I"(SC_CACHECLEARE), "r"(addr), "r"(len), "r"(c):"memory","lr");

    AROS_LIBFUNC_EXIT
} /* CacheClearE */
