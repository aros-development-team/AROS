/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Tell the position in a stream
    Lang: english
*/
#include <errno.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include "__errno.h"
#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	long ftell (

/*  SYNOPSIS */
	FILE * stream)

/*  FUNCTION
	Tell the current position in a stream.

    INPUTS
	stream - Obtain position of this stream

    RESULT
	The position on success and -1 on error.
	If an error occurred, the global variable errno is set.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	fopen(), fseek(), fwrite()

    INTERNALS

    HISTORY

******************************************************************************/
{
    long cnt;
    BPTR fh;

    if (((IPTR)stream) < 3)
    {
	errno = EINVAL;
	return -1;
    }

    fh = (BPTR)(stream->fh);

    Flush (fh);
    cnt = Seek (fh, 0, OFFSET_CURRENT);

    if (cnt == -1)
    {
	errno = IoErr2errno (IoErr ());
    }

    return cnt;
} /* ftell */
