/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GetCC() - Read the CPU condition codes in an easy way.
    Lang: english
*/

#include <aros/debug.h>

#include <proto/exec.h>

/* See rom/exec/getcc.c for documentation */

AROS_LH0(UWORD, GetCC,
    struct ExecBase *, SysBase, 88, Exec)
{
    AROS_LIBFUNC_INIT

    /* Overridden on the RegCall ABI's startup routine */
    aros_print_not_implemented("GetCC");
    return ~0;

    AROS_LIBFUNC_EXIT
} /* GetCC() */
