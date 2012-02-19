/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function scanf().
*/

#include <stdarg.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

	int scanf (

/*  SYNOPSIS */
	const char * format,
	...)

/*  FUNCTION

    INPUTS

    RESULT
	The number of converted parameters

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	fscanf(), vscanf(), vfscanf(), sscanf(), vsscanf()

    INTERNALS

******************************************************************************/
{
    int     retval;
    va_list args;

    va_start (args, format);

    retval = vfscanf (stdin, format, args);

    va_end (args);

    fflush (stdout);

    return retval;
} /* scanf */

