/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    Desc: CacheClearE() - Clear the caches with extended control.
*/

#include <aros/config.h>

#include <proto/exec.h>

#include <aros/libcall.h>
#include <exec/types.h>
#include <exec/execbase.h>

#include "kernel_syscall.h"


/* See rom/exec/cachecleare.c for documentation */

AROS_LH3(void, CacheClearE,
    AROS_LHA(APTR, address, A0),
    AROS_LHA(ULONG, length, D0),
    AROS_LHA(ULONG, caches, D1),
    struct ExecBase *, SysBase, 107, Exec)
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
} /* CacheClearE */
