/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs stub for Printf()
    Lang: english
*/

#include <dos/dos.h>
#include <proto/dos.h>

LONG Printf(STRPTR fmt, ...)
{
    LONG retval;
    AROS_SLOWSTACKMETHODS_PRE(fmt)

    retval = VFPrintf(Output(), fmt, AROS_SLOWSTACKMETHODS_ARG(fmt));

    AROS_SLOWSTACKMETHODS_POST
    return retval;
}
