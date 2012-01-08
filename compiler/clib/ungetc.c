/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function ungetc().
*/

#include <errno.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "__fdesc.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	int ungetc (

/*  SYNOPSIS */
	int    c,
	FILE * stream)

/*  FUNCTION
	Push the character c character back into the stream.

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
    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
	stream->flags |= _STDIO_ERROR;
	errno = EBADF;
	return EOF;
    }

    /* Note: changes here might require changes in vfscanf.c!! */

    if (c < -1)
	c = (unsigned int)c;

    if (!UnGetC ((BPTR)fdesc->fcb->fh, c))
    {
	errno = __arosc_ioerr2errno (IoErr ());

	if (errno)
	    stream->flags |= _STDIO_ERROR;
	else
	    stream->flags |= _STDIO_EOF;

	c = EOF;
    }

    return c;
} /* ungetc */

