/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Format a string and call a usercallback to output each char
    Lang: english
*/
/* Original source from libnix */

#define AROS_ALMOST_COMPATIBLE

/*****************************************************************************

    NAME */
#include <stdio.h>
#include <stdarg.h>

	int vfprintf (

/*  SYNOPSIS */
	FILE	   * stream,
	const char * format,
	va_list      args)

/*  FUNCTION
	Format a list of arguments and print them on the specified stream.

    INPUTS
	stream - A stream on which one can write
	format - A printf() format string.
	args - A list of arguments for the format string.

    RESULT
	The number of characters written.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	06.12.1996 digulla copied from libnix

******************************************************************************/
{
    return __vcformat (stream, (void *)fputc, format, args);
} /* vfprintf */
