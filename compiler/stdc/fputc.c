/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fputc().
*/
#include <dos/dos.h>
#include <proto/dos.h>
#include <errno.h>

#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	int fputc (

/*  SYNOPSIS */
	int    c,
	FILE * stream)

/*  FUNCTION
	Write one character to the specified stream.

    INPUTS
	c - The character to output
	stream - The character is written to this stream

    RESULT
	The character written or EOF on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    if (!(stream->flags & __STDCIO_STDIO_WRITE))
    {
        SetIoErr(ERROR_WRITE_PROTECTED);
        errno = EACCES;
        stream->flags |= __STDCIO_STDIO_ERROR;
        return EOF;
    }

    if ((stream->flags & __STDCIO_STDIO_APPEND))
        Seek(stream->fh, 0, OFFSET_END);

    c = (int)FPutC(stream->fh, c);

    if (c == EOF)
    {
	errno = __stdc_ioerr2errno(IoErr());
        stream->flags |= __STDCIO_STDIO_ERROR;
    }

    return c;
} /* fputc */
