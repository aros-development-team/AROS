/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function vscanf()
*/
#include <stdarg.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

	int vscanf (

/*  SYNOPSIS */
	const char * format,
	va_list      args)

/*  FUNCTION
	Scan the standard input and convert it into the arguments as
	specified by format.

    INPUTS
	format - A scanf() format string.
	args - A list of arguments for the results

    RESULT
	The number of converted parameters.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    return vfscanf (stdin, format, args);
} /* vscanf */

