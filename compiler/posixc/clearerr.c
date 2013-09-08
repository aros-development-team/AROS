/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function clearerr().
*/

#include "__stdio.h"

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
    stream->flags &= ~(__POSIXC_STDIO_EOF | __POSIXC_STDIO_ERROR);
} /* clearerr */

