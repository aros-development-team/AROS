/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function ferror()
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <stdio.h>

	int ferror (

/*  SYNOPSIS */
	FILE * stream)

/*  FUNCTION
	Test the error flag of a stream. This flag is set automatically by
	any function that detects an error. To clear it, call clearerr().

    INPUTS
	stream - The stream to be tested.

    RESULT
	!= 0, if the stream had an error, 0 otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	ferror(), clearerr()

    INTERNALS

    HISTORY

******************************************************************************/
{
    return (stream->flags & _STDIO_ERROR) != 0;
} /* ferror */

