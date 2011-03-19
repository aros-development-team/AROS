/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/rawfmt.h>
#include <proto/exec.h>
#include <stdarg.h>

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
    va_list args;

    va_start(args, format);
    VNewRawDoFmt(format, RAWFMTFUNC_STRING, buffer, args);
    va_end(args);
} /* sprintf */
