/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs stub for VFWritef()
    Lang: english
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <stdarg.h>

LONG FWritef(BPTR fh, STRPTR fmt, ...)
{
    LONG retval;

    retval = VFWritef(fh, fmt, (LONG *)(&fmt+1));

    return retval;
}
