/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function feof()
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <stdio.h>

	int feof (

/*  SYNOPSIS */
	FILE * stream)

/*  FUNCTION
	Test the EOF-Flag of a stream. This flag is set automatically by
	any function which recognises EOF. To clear it, call clearerr().

    INPUTS
	stream - The stream to be tested.

    RESULT
	!= 0, if the stream is at the end of the file, 0 otherwise.

    NOTES
        This function must not be used in a shared library or
        in a threaded application.

    EXAMPLE

    BUGS

    SEE ALSO
	ferror(), clearerr()

    INTERNALS

    HISTORY

******************************************************************************/
{
    return (stream->flags & _STDIO_FILEFLAG_EOF) != 0;
} /* feof */

