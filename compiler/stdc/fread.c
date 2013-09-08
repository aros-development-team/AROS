/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fread().
*/
#include <proto/dos.h>
#include <errno.h>

#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	size_t fread (

/*  SYNOPSIS */
	void * restrict buf,
	size_t size,
	size_t nblocks,
	FILE * restrict stream)

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
    LONG cnt;

    if (size == 0 || nblocks == 0)
        return 0;

    if (!(stream->flags & __STDCIO_STDIO_READ))
    {
        SetIoErr(ERROR_READ_PROTECTED);
        errno = EACCES;
        stream->flags |= __STDCIO_STDIO_ERROR;
        return 0;
    }

    cnt = FRead (stream->fh, buf, size, nblocks);

    if (cnt == -1)
    {
	errno = __stdc_ioerr2errno (IoErr ());
	stream->flags |= __STDCIO_STDIO_ERROR;

	cnt = 0;
    }
    else if (cnt < nblocks)
    {
	stream->flags |= __STDCIO_STDIO_EOF;
    }

    return (size_t)cnt;
} /* fread */
