/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function ungetc()
    Lang: english
*/

#include <errno.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "__errno.h"
#include "__open.h"

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

    HISTORY
	10.12.1996 digulla created

******************************************************************************/
{
    GETUSER;

    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
	stream->flags |= _STDIO_ERROR;
	errno = EBADF;
	return EOF;
    }

    if (c < -1)
	c = (unsigned int)c;

    if (!UnGetC ((BPTR)fdesc->fh, c))
    {
	errno = IoErr2errno (IoErr ());

	if (errno)
	    stream->flags |= _STDIO_ERROR;
	else
	    stream->flags |= _STDIO_EOF;

	return EOF;
    }

    return c;
} /* ungetc */

