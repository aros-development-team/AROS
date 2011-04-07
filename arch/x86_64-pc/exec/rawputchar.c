/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Emit one character via raw IO
    Lang: english
*/

#include <aros/libcall.h>
#include <exec/types.h>

/* This is from kernel.resource. Temporary hack until the kernel is rewritten. */
int kputc(int c, void *data);

AROS_LH1(void, RawPutChar,
	 AROS_LHA(UBYTE, chr, D0),
	 struct ExecBase *, SysBase, 86, Exec)
{
    AROS_LIBFUNC_INIT

    kputc(chr, NULL);

    AROS_LIBFUNC_EXIT
} /* RawPutChar */
