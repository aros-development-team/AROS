/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function fflush()
    Lang: english
*/
#include <exec/types.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "__errno.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	int fflush (

/*  SYNOPSIS */
	FILE * stream)

/*  FUNCTION
	Flush a stream. If the stream is an input stream, then the stream
	is synchronised for unbuffered I/O. If the stream is an output
	stream, then any buffered data is written.

    INPUTS
	stream - Flush this stream. May be NULL. In this case, all
		output streams are flushed.

    RESULT
	0 on success or EOF on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	10.12.1996 digulla created

******************************************************************************/
{
    BPTR fh;

#warning TODO: flush all output streams

	fh = (BPTR)stream->fh;

    if (fh && Flush (fh))
		return 0;

    if (!fh)
		errno = EINVAL;
    else
		errno = IoErr2errno(IoErr());

    return EOF;
} /* fflush */

