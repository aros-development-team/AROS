/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function vprintf().
*/
#include <libraries/stdcio.h>

/*****************************************************************************

    NAME */
#include <stdio.h>
#include <stdarg.h>

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
    struct StdCIOBase *StdCIOBase = __aros_getbase_StdCIOBase();

    return vfprintf (StdCIOBase->_stdout, format, args);
} /* vprintf */
