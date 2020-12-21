/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Format a string and call a usercallback to output each char.
*/
/* Original source from libnix */
#include <stdarg.h>

#include <aros/debug.h>

#include "debug.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	int vfprintf (

/*  SYNOPSIS */
	FILE	   * restrict stream,
	const char * restrict format,
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

******************************************************************************/
{
    D(bug("[%s] %s: Calling __vcformat(stream = 0x%p, fputc = 0x%p, format = '%s', ...)\n", STDCNAME, __func__,
          stream, fputc, format
    ));

    return __vcformat (stream, (int (*)(int, void *))fputc, format, args);
} /* vfprintf */
