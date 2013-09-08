/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function scanf().
*/
#include <libraries/stdcio.h>

/*****************************************************************************

    NAME */
#include <stdio.h>
#include <stdarg.h>

	int scanf (

/*  SYNOPSIS */
	const char * restrict format,
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
    struct StdCIOBase *StdCIOBase = __aros_getbase_StdCIOBase();
    int     retval;
    va_list args;

    va_start (args, format);

    retval = vfscanf (StdCIOBase->_stdin, format, args);

    va_end (args);

    return retval;
} /* scanf */
