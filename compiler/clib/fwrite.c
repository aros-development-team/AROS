/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function fwrite().
*/

#include <errno.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/debug.h>
#include "__errno.h"
#include "__stdio.h"
#include "__open.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

	size_t fwrite (

/*  SYNOPSIS */
	const void * restrict	buf,
	size_t			size,
	size_t			nblocks,
	FILE * restrict		stream)

/*  FUNCTION
        Write an amount of bytes to a stream.

    INPUTS
        buf - The buffer to write to the stream
        size - Size of one block to write
        nblocks - The number of blocks to write
        stream - Write to this stream

    RESULT
        The number of blocks written. If no error occurred, this is
        nblocks. Otherwise examine errno for the reason of the error.

    SEE ALSO
        fopen(), fwrite()

******************************************************************************/
{
    size_t cnt;

    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
        errno = EBADF;

        return 0;
    }

    cnt = FWrite ((BPTR)fdesc->fh, (CONST APTR)buf, size, nblocks);

    if (cnt == -1)
    {
        errno = IoErr2errno (IoErr ());
        
        cnt = 0;
    }

    return cnt;
} /* fwrite */

