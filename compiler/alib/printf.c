/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs stub for Printf()
    Lang: english
*/

#include <dos/dos.h>
#include <proto/dos.h>

LONG Printf(STRPTR fmt, ...) __stackparm;

LONG Printf(STRPTR fmt, ...)
{
    LONG retval;

    retval = VFPrintf(Output(), fmt, &fmt+1);

    return retval;
}
