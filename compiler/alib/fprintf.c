/*
    Copyright (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Varargs stub for VFPrintf()
    Lang: english
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <stdarg.h>

void FPrintf(BPTR fh, STRPTR fmt, LONG arg, ...)
{
    va_list args;
    va_start(args,arg);

    VFPrintf(fh, fmt, (LONG *)args);

    va_end(args);
}
