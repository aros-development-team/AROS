/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: CacheClearU() for 64bit RISC-V.
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*
    See rom/exec/cacheclearu.c for the documentation, and cachecleare.c
    for why this does so little and what replaces it.
*/

AROS_LH0(void, CacheClearU,
    struct ExecBase *, SysBase, 106, Exec)
{
    AROS_LIBFUNC_INIT

    __asm__ __volatile__ ("fence rw, rw" ::: "memory");

    AROS_LIBFUNC_EXIT
} /* CacheClearU */
