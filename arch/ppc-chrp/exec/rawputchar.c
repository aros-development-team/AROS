/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Emit one character via raw IO
    Lang: english
*/

#include <aros/libcall.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <stdarg.h>

#include "exec_intern.h"

#undef bug

/* See rom/exec/rawputchar.c for documentation */

static inline void bug(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    KrnBug(format, args);
    va_end(args);
}


AROS_LH1(void, RawPutChar,
    AROS_LHA(UBYTE, chr, D0),
    struct ExecBase *, SysBase, 86, Exec)
{
    AROS_LIBFUNC_INIT

    /* Don't write 0 bytes */
    if (chr)
    {
        bug("%c", chr);
    }

    AROS_LIBFUNC_EXIT
} /* RawPutChar */
