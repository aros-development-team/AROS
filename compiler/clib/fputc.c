/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function fputc()
    Lang: english
*/

#include <errno.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "__open.h"

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

    HISTORY
	06.12.1996 digulla created

******************************************************************************/
{
    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
        GETUSER;

    	errno = EBADF;
	return EOF;
    }

    if (FPutC((BPTR)fdesc->fh, c) == EOF)
    {
        GETUSER;

	errno = IoErr2errno(IoErr());
	c = EOF;
    }

    return c;
} /* fputc */

