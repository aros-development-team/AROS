/*
    Copyright (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Varargs stub for VFPrintf()
    Lang: english
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <stdarg.h>

LONG FPrintf(BPTR fh, STRPTR fmt, ...)
{
    LONG retval;
    va_list args;

    va_start(args,fmt);

    retval = VFPrintf(fh, fmt, (LONG *)args);

    va_end(args);
    return retval;
}
