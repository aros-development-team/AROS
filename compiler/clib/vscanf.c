/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Implementation of vscanf()
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <stdio.h>
#include <stdarg.h>

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

    HISTORY
	06.12.1996 digulla created

******************************************************************************/
{
    GETUSER;

    return vfscanf (stdin, format, args);
} /* vscanf */

