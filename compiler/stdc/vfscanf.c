/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Scan a stream and write the result in the parameters.
*/
/* Original source from libnix */
#include <proto/dos.h>
#include <stdarg.h>

#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	int vfscanf (

/*  SYNOPSIS */
	FILE	   * stream,
	const char * format,
	va_list      args)

/*  FUNCTION
	Read the scream, scan it as the format specified and write the
	result of the conversion into the specified arguments.

    INPUTS
	stream - A stream to read from
	format - A scanf() format string.
	args - A list of arguments for the results.

    RESULT
	The number of converted arguments.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    Flush (stream->fh);
    
    return __vcscan (stream, (int (*)(void *))fgetc, (int (*)(int, void *))ungetc, format, args);
} /* vfscanf */
