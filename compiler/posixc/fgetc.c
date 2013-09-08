/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fgetc().
*/

#include <errno.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "__fdesc.h"
#include "__stdio.h"

#define DEBUG 0
#include <aros/debug.h>

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

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	getc(), fputc(), putc()

    INTERNALS

******************************************************************************/
{
    int c;
    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
        errno = EBADF;
	stream->flags |= _STDIO_ERROR;
	return EOF;
    }

    /* Note: changes here might require changes in vfscanf.c!! */

    if (fdesc->fcb->privflags & _FCB_FLUSHONREAD)
    {
        D(bug("[fgetc]privflags before flush 0x%x\n", fdesc->fcb->privflags));
        fdesc->fcb->privflags &= ~_FCB_FLUSHONREAD;
        D(bug("[fgetc]privflags after flush 0x%x\n", fdesc->fcb->privflags));
        Flush(fdesc->fcb->fh);
    }

    c = FGetC ((BPTR)(fdesc->fcb->fh));
    if (c == EOF)
    {
	c = IoErr ();

	if (c)
	{
    	    errno = __arosc_ioerr2errno (c);

	    stream->flags |= _STDIO_ERROR;
	}
	else
	    stream->flags |= _STDIO_EOF;

	c = EOF;
    }

    return c;
} /* fgetc */

