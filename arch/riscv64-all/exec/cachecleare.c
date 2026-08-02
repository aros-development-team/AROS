/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: CacheClearE() for 64bit RISC-V.
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*
    See rom/exec/cachecleare.c for the documentation.

    The generic version, needing no extension beyond the base ISA: it
    orders what has already been written, and no more. That is all a
    machine whose instruction fetch is coherent with its data side
    requires.

    Where it is not coherent, something has to reconcile the two - RISC-V
    keeps them apart on purpose, so a program just read from disk stays
    data until told otherwise. What can do the telling differs per
    machine, so the platform detects what it has at startup and replaces
    this with it; see arch/riscv64-opensbi/exec/cpu_init.c for one.
*/

AROS_LH3(void, CacheClearE,
    AROS_LHA(APTR,  address, A0),
    AROS_LHA(IPTR,  length,  D0),
    AROS_LHA(ULONG, caches,  D1),
    struct ExecBase *, SysBase, 107, Exec)
{
    AROS_LIBFUNC_INIT

    __asm__ __volatile__ ("fence rw, rw" ::: "memory");

    AROS_LIBFUNC_EXIT
} /* CacheClearE */
