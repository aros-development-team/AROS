/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fgets().
*/
#include <proto/dos.h>
#include <errno.h>

#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	char * fgets (

/*  SYNOPSIS */
	char * buffer,
	int    size,
	FILE * stream)

/*  FUNCTION
	Read one line of characters from the stream into the buffer.
	Reading will stop, when a newline ('\n') is encountered, EOF
	or when the buffer is full. If a newline is read, then it is
	put into the buffer. The last character in the buffer is always
	'\0' (Therefore at most size-1 characters can be read in one go).

    INPUTS
	buffer - Write characters into this buffer
	size - This is the size of the buffer in characters.
	stream - Read from this stream

    RESULT
	buffer or NULL in case of an error or EOF.

    NOTES

    EXAMPLE
	// Read a file line by line
	char line[256];

	// Read until EOF
	while (fgets (line, sizeof (line), fh))
	{
	    // Evaluate the line
	}

    BUGS

    SEE ALSO
	fopen(), gets(), fputs(), putc()

    INTERNALS

******************************************************************************/
{
    if (!(stream->flags & __STDCIO_STDIO_READ))
    {
        SetIoErr(ERROR_READ_PROTECTED);
        errno = EACCES;
        stream->flags |= __STDCIO_STDIO_ERROR;
        return NULL;
    }

    buffer = FGets (stream->fh, buffer, size);

    if (!buffer)
    {
        LONG ioerr = IoErr();

	if (ioerr)
	{
	    errno = __stdc_ioerr2errno(ioerr);
	    stream->flags |= __STDCIO_STDIO_ERROR;
        }
	else
	{
	    stream->flags |= __STDCIO_STDIO_EOF;
	}
    }

    return buffer;
} /* fgets */
