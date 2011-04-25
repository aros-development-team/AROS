/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Low-level debugging support.
*/

#include <exec/rawfmt.h>
#include <proto/exec.h>
#include <aros/asmcall.h>
#include <stdarg.h>

void KPrintF(STRPTR format, ...)
{
    va_list ap;

    va_start(ap, format);

    VNewRawDoFmt(format, (APTR)RAWFMTFUNC_SERIAL, NULL, ap);

    va_end(ap);
}
