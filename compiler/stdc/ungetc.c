/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function ungetc().
*/
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <errno.h>

#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	int ungetc (

/*  SYNOPSIS */
	int    c,
	FILE * stream)

/*  FUNCTION
	Puch the character c character back into the stream.

    INPUTS
	c - Put this character back into the stream. The next read will
		return this character. If you push back more than one
		character, then they will be returned in reverse order.
		The function gurantees that one character can be
		pushed back but no more. It is possible to push the EOF
		character back into the stream.
	stream - Read from this stream

    RESULT
	c or EOF on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	fgetc(), getc(), fputc(), putc()

    INTERNALS

******************************************************************************/
{
    /* Note: changes here might require changes in vfscanf.c!! */

    if (c < -1)
	c = (unsigned int)c;

    if (!UnGetC (stream->fh, c))
    {
        LONG ioerr = IoErr();

	if (ioerr)
        {
            errno = __stdc_ioerr2errno(ioerr);
	    stream->flags |= __STDCIO_STDIO_ERROR;
        }
	else
	    stream->flags |= __STDCIO_STDIO_EOF;

	c = EOF;
    }

    return c;
} /* ungetc */
