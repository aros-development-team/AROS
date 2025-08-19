/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    Desc: Varargs stub for VFPrintf()
*/

#define NO_INLINE_STDARG /* turn off inline def */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>

LONG    FPrintf(BPTR fh, CONST_STRPTR fmt, ...) __stackparm;

LONG    FPrintf(BPTR fh, CONST_STRPTR fmt, ...)
{
    LONG retval;

    AROS_SLOWSTACKFORMAT_PRE(fmt);
    retval = VFPrintf(fh, fmt, AROS_SLOWSTACKFORMAT_ARG(fmt));
    AROS_SLOWSTACKFORMAT_POST(fmt);

    return retval;
}
