/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function fgetc()
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

    HISTORY
	10.12.1996 digulla created

******************************************************************************/
{
    GETUSER;

    int c;
    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
        errno = EBADF;
	stream->flags |= _STDIO_ERROR;
	return EOF;
    }

    c = FGetC ((BPTR)(fdesc->fh));
    if (c == EOF)
    {
	c = IoErr ();

	if (c)
	{
    	    errno = IoErr2errno (c);

	    stream->flags |= _STDIO_ERROR;
	}
	else
	    stream->flags |= _STDIO_EOF;

	c = EOF;
    }

    return c;
} /* fgetc */

