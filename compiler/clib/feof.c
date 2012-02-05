/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function feof().
*/

#include "__stdio.h"

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

******************************************************************************/
{
    return (stream->flags & _STDIO_EOF) != 0;
} /* feof */

