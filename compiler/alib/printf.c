/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs stub for Printf()
    Lang: english
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <stdarg.h>

LONG Printf(STRPTR fmt, ...)
{
    LONG retval;
    va_list args;

    va_start(args,fmt);

    retval = VFPrintf(Output(), fmt, (LONG *)args);

    va_end(args);
    return retval;
}
