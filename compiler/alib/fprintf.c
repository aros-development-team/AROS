/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Print a formatted string to a specific file
    Lang: english
*/

#include "alib_intern.h"

/*****************************************************************************

    NAME */
#include <exec/types.h>
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/dos.h>

	LONG FPrintf (

/*  SYNOPSIS */
	BPTR   fh,
	STRPTR fmt,
	...    )

/*  FUNCTION

    INPUTS
	fh - the file to print to
	fmt - the string to print
	arg1 - the first argument to the formatted string

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
  return 0L;
} /* FPrintf */
