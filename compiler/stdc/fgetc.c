/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fgetc().
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <errno.h>

#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	int fgetc (

/*  SYNOPSIS */
	FILE * stream)

/*  FUNCTION
	Read one character from the stream. If there is no character
	available or an error occurred, the function returns EOF.

    INPUTS
	stream - Read from this stream

    RESULT
	The character read or EOF on end of file or error.
        If EOF is returned feof() and ferror() indicate if it was an
        end-of-file situation or an error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	getc(), feof(), ferror(), fputc(), putc()

    INTERNALS

******************************************************************************/
{
    LONG c, ioerr;

    if (!(stream->flags & __STDCIO_STDIO_READ))
    {
        SetIoErr(ERROR_READ_PROTECTED);
        errno = EACCES;
        stream->flags |= __STDCIO_STDIO_ERROR;
        return EOF;
    }

    if (stream->flags & __STDCIO_STDIO_FLUSHONREAD)
    {
        Flush(stream->fh);
        stream->flags &= ~__STDCIO_STDIO_FLUSHONREAD;
    }

    c = FGetC (stream->fh);
    if (c == EOF)
    {
	ioerr = IoErr ();

	if (ioerr)
	{
    	    errno = __stdc_ioerr2errno (ioerr);
	    stream->flags |= __STDCIO_STDIO_ERROR;
	}
	else
	    stream->flags |= __STDCIO_STDIO_EOF;
    }

    return (int)c;
} /* fgetc */
