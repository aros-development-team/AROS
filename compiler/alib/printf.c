/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Print a formatted string
    Lang: english
*/

#include "alib_intern.h"

/*****************************************************************************

    NAME */
#include <exec/types.h>
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/dos.h>

	LONG Printf (

/*  SYNOPSIS */
	STRPTR fmt,
	...    )

/*  FUNCTION

    INPUTS
	fmt - the string to print
	arg1 - the first argument to the formatted string

    RESULT

    NOTES

    EXAMPLE

    BUGS
	This is a quick hack. It doesn't work on 64bit nor when the stack
	is reversed.

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    IPTR * args;

    args = ((IPTR *)fmt) + 1;
    return VPrintf (fmt, args);
} /* Printf */
