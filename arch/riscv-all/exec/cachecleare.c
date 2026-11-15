/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: CacheClearE() for RISC-V.
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*
    See rom/exec/cachecleare.c for the documentation.

    A placeholder that every platform has to replace with its own flush.
    It needs no extension beyond the base ISA: it orders what has already
    been written, and no more, so it does nothing for the instruction
    cache.

    RISC-V keeps instruction fetch apart from the data side on purpose,
    so a program just read from disk stays data until told otherwise.
    What can do the telling differs per machine, so each platform detects
    what it has at startup and replaces this function (and CacheClearU())
    with it.
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
