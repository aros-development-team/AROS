/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Change the position in a stream
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <stdio.h>

	void rewind (

/*  SYNOPSIS */
	FILE * stream)

/*  FUNCTION
	Change the current position in a stream to the beginning.

    INPUTS
	stream - Modify this stream

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	fopen(), fwrite(), fseek()

    INTERNALS

    HISTORY

******************************************************************************/
{
    fseek (stream, 0L, SEEK_SET);
    clearerr (stream);
} /* rewind */

