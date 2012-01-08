/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Tell the position in a stream.
*/

#include <errno.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include "__stdio.h"
#include "__fdesc.h"

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

******************************************************************************/
{
    long cnt;
    BPTR fh;
    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
	errno = EBADF;
	return 0;
    }

    fh = (BPTR)(fdesc->fcb->fh);

    Flush (fh);
    cnt = Seek (fh, 0, OFFSET_CURRENT);

    if (cnt == -1)
	errno = __arosc_ioerr2errno (IoErr ());

    return cnt;
} /* ftell */
