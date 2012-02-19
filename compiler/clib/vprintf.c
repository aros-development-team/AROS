/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Implementation of vprintf().
*/
#include <stdarg.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

	int vprintf (

/*  SYNOPSIS */
	const char * format,
	va_list      args)

/*  FUNCTION
	Format a list of arguments and print them on the standard output.

    INPUTS
	format - A printf() format string.
	args - A list of arguments for the format string.

    RESULT
	The number of characters written.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    return vfprintf (stdout, format, args);
} /* vprintf */

