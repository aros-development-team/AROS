/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    C99 function rewind()
*/

/*****************************************************************************

    NAME */
#include <stdio.h>

	void rewind (

/*  SYNOPSIS */
	FILE * stream)

/*  FUNCTION
	Change the current position in a stream to the beginning.
        It also clears the error indication of the stream.

    INPUTS
	stream - Modify this stream

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	fopen(), fwrite(), fseek()

    INTERNALS

******************************************************************************/
{
    fseek (stream, 0L, SEEK_SET);
    clearerr (stream);
} /* rewind */

