/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fread().
*/

#include <errno.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "__stdio.h"
#include "__fdesc.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	size_t fread (

/*  SYNOPSIS */
	void * buf,
	size_t size,
	size_t nblocks,
	FILE * stream)

/*  FUNCTION
	Read an amount of bytes from a stream.

    INPUTS
	buf - The buffer to read the bytes into
	size - Size of one block to read
	nblocks - The number of blocks to read
	stream - Read from this stream

    RESULT
	The number of blocks read. This may range from 0 when the stream
	contains no more blocks up to nblocks. In case of an error, 0 is
	returned.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	fopen(), fwrite()

    INTERNALS

******************************************************************************/
{
    size_t cnt;
    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
	stream->flags |= __POSIXC_STDIO_ERROR;
	errno = EBADF;
	return 0;
    }

    cnt = FRead ((BPTR)fdesc->fcb->fh, buf, size, nblocks);

    if (cnt == -1)
    {
	errno = __stdc_ioerr2errno (IoErr ());
	stream->flags |= __POSIXC_STDIO_ERROR;

	cnt = 0;
    }
    else if (cnt == 0 || cnt < nblocks)
    {
	stream->flags |= __POSIXC_STDIO_EOF;
    }

    return cnt;
} /* fread */

