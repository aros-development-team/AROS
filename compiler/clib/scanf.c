/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function scanf()
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <stdio.h>
#include <stdarg.h>

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
	fscanf(), vscanf(), vfscanf(), sscanf(), vsscanf(),
	vnsscanf()

    INTERNALS

    HISTORY
	28.01.1997 digulla created

******************************************************************************/
{
    GETUSER;

    int     retval;
    va_list args;

    va_start (args, format);

    retval = vfscanf (stdin, format, args);

    va_end (args);

    fflush (stdout);

    return retval;
} /* scanf */

