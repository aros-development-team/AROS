/*
    (C) 1995-96 AROS - The Amiga Replacement OS
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
		pushed back but no more. The EOF character cannot be
		pushed back.
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
    BPTR fh;

    switch ((IPTR)stream)
    {
    case 1: /* Stdin */
	fh = Input ();
	break;

    case 2: /* Stdout */
    case 3: /* Stderr */
	errno = EINVAL;
	return EOF;

    default:
	fh = (BPTR)stream->fh;
	break;
    }

    if (c == EOF)
	return EOF;

    if (!UnGetC (fh, c))
    {
	errno = IoErr2errno (IoErr ());

	if (errno)
	    stream->flags |= _STDIO_FILEFLAG_ERROR;
	else
	    stream->flags |= _STDIO_FILEFLAG_EOF;

	return EOF;
    }

    return c;
} /* ungetc */

