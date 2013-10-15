/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fwrite().
*/

#include <errno.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#define DEBUG 0
#include <aros/debug.h>
#include "__stdio.h"
#include "__fdesc.h"

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

    D(bug("[fwrite]: buf=%p, size=%d, nblocks=%d, stream=%p\n",
          buf, size, nblocks, stream
    ));

    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
        errno = EBADF;

        return 0;
    }

    if (nblocks > 0 && size > 0)
	cnt = FWrite (fdesc->fcb->handle, (CONST APTR)buf, size, nblocks);
    else
	cnt = 0;

    if (cnt == -1)
    {
        errno = __stdc_ioerr2errno (IoErr ());
        
        cnt = 0;
    }

    return cnt;
} /* fwrite */

