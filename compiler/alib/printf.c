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

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
  return 0L;
} /* Printf */
