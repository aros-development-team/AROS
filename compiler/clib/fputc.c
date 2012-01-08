/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function fputc().
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
    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
    	errno = EBADF;
	return EOF;
    }

    if (FPutC((BPTR)fdesc->fcb->fh, c) == EOF)
    {
	errno = __arosc_ioerr2errno(IoErr());
	c = EOF;
    }

    return c;
} /* fputc */

