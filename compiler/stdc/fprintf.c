/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fprintf().
*/

#include <stdarg.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

	int fprintf (

/*  SYNOPSIS */
	FILE	   * restrict fh,
	const char * restrict format,
	...)

/*  FUNCTION
	Format a string with the specified arguments and write it to
	the stream.

    INPUTS
	fh - Write to this stream
	format - How to format the arguments
	... - The additional arguments

    RESULT
	The number of characters written to the stream or EOF on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    int     retval;
    va_list args;

    va_start (args, format);

    retval = vfprintf (fh, format, args);

    va_end (args);

    return retval;
} /* fprintf */
