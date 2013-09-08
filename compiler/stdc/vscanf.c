/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function vscanf().
*/
#include <libraries/stdcio.h>

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

******************************************************************************/
{
    struct StdCIOBase *StdCIOBase = __aros_getbase_StdCIOBase();

    return vfscanf (StdCIOBase->_stdin, format, args);
} /* vscanf */
