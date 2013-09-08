/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fwrite().
*/
#include <proto/dos.h>
#include <errno.h>

#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

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
    LONG cnt;

    if (size == 0 || nblocks == 0)
        return 0;

    if (!(stream->flags & __STDCIO_STDIO_WRITE))
    {
        SetIoErr(ERROR_WRITE_PROTECTED);
        errno = EACCES;
        stream->flags |= __STDCIO_STDIO_ERROR;
        return 0;
    }

    if ((stream->flags & __STDCIO_STDIO_APPEND))
        Seek(stream->fh, 0, OFFSET_END);

    cnt = FWrite (stream->fh, (CONST APTR)buf, size, nblocks);

    if (cnt == -1)
    {
        errno = __stdc_ioerr2errno (IoErr ());
        stream->flags |= __STDCIO_STDIO_ERROR;

        cnt = 0;
    }

    return (size_t)cnt;
} /* fwrite */
