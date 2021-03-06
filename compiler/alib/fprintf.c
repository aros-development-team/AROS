/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc: Varargs stub for VFPrintf()
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <stdarg.h>

LONG FPrintf(BPTR fh, STRPTR fmt, ...)
{
    LONG retval;

    retval = VFPrintf(fh, fmt, (LONG *)(&fmt+1));

    return retval;
}
