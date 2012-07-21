/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/rawfmt.h>
#include <proto/exec.h>
#include <stdarg.h>

#ifdef __mc68000
const ULONG m68k_string_sprintf = 0x16c04e75;
#endif

/*****************************************************************************

    NAME */
#include <proto/alib.h>

       VOID  __sprintf(

/*  SYNOPSIS */
	     UBYTE *buffer, const UBYTE *format, ...)

/*  FUNCTION
	Print a formatted string to a buffer.

    INPUTS
	buffer   --  the buffer to fill
	format   --  the format string, see the VNewRawDoFmt() documentation for
		     information on which formatting commands there are

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	exec.library/VNewRawDoFmt()

    INTERNALS

    HISTORY
	07.01.2000  SDuvan  implemented

*****************************************************************************/
{
#ifdef __mc68000
    /* Special case for m68k, so that we are AmigaOS 1.x/2.x compliant
     * New programs should be using snprintf() from arosc.library
     */
    RawDoFmt(format, &format+1, (VOID_FUNC)&m68k_string_sprintf, buffer);
#else
    va_list args;

    va_start(args, format);
    VNewRawDoFmt(format, RAWFMTFUNC_STRING, buffer, args);
    va_end(args);
#endif
} /* sprintf */
