/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fputs().
*/
#include <proto/dos.h>
#include <errno.h>

#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	int fputs (

/*  SYNOPSIS */
	const char * str,
	FILE	   * stream)

/*  FUNCTION
	Write a string to the specified stream.

    INPUTS
	str - Output this string...
	fh - ...to this stream

    RESULT
	> 0 on success and EOF on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	puts(), fputc(), putc()

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

    if (!str) str = "(null)";

    if (FPuts(stream->fh, str) == -1)
    {
	errno = __stdc_ioerr2errno(IoErr());
	return EOF;
    }

    return 0;
} /* fputs */
