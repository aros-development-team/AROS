/*
    Copyright � 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function clearerr().
*/

/*****************************************************************************

    NAME */
#include <stdio.h>

	void clearerr (

/*  SYNOPSIS */
	FILE * stream)

/*  FUNCTION
	Clear EOF and error flag in a stream. You must call this for
	example after you have read the file until EOF, then appended
	something to it and want to continue reading.

    INPUTS
	stream - The stream to be reset.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	ferror(), clearerr()

    INTERNALS

******************************************************************************/
{
    stream->flags &= ~(_STDIO_EOF | _STDIO_ERROR);
} /* clearerr */

