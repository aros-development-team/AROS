/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function fwrite()
    Lang: english
*/

#include <errno.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "__errno.h"
#include "__stdio.h"
#include "__open.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

	size_t fwrite (

/*  SYNOPSIS */
	void * buf,
	size_t size,
	size_t nblocks,
	FILE * stream)

/*  FUNCTION
	Read an amount of bytes from a stream.

    INPUTS
	buf - The buffer to write to the stream
	size - Size of one block to write
	nblocks - The number of blocks to write
	stream - Write to this stream

    RESULT
	The number of blocks written. If no error occurred, this is
	nblocks. Otherwise examine errno for the reason of the error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	fopen(), fwrite()

    INTERNALS

    HISTORY
	15.12.1996 digulla created

******************************************************************************/
{
    size_t cnt;

    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
        GETUSER;

	errno = EBADF;
	return 0;
    }

    cnt = FWrite ((BPTR)fdesc->fh, buf, size, nblocks);

    if (cnt == -1)
    {
    	GETUSER;

	errno = IoErr2errno (IoErr ());

	cnt = 0;
    }

    return cnt;
} /* fwrite */

