/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function fflush().
*/

#include "__arosc_privdata.h"

#include <exec/types.h>
#include <exec/lists.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "__errno.h"
#include "__stdio.h"
#include "__open.h"

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

******************************************************************************/
{
    AROS_GET_SYSBASE_OK
    AROS_GET_DOSBASE

    /* flush all streams opened for output */
    if (!stream)
    {
	FILENODE *fn;

	ForeachNode (&__stdio_files, fn)
	{
	    if (fn->File.flags & _STDIO_WRITE)
	    {
	    	fdesc *fdesc = __getfdesc(fn->File.fd);

		if (!fdesc)
		{
		    errno = EBADF;
		    return EOF;
      		}

		if (!Flush((BPTR)fdesc->fh))
		{
		    errno = IoErr2errno(IoErr());
		    return EOF;
      		}
            }
        }
    }
    else
    {
    	fdesc *fdesc = __getfdesc(stream->fd);

	if (!fdesc || !(stream->flags & _STDIO_WRITE))
	{
	    errno = EBADF;
	    return EOF;
	}

	if (Flush((BPTR)fdesc->fh))
	    return 0;
    }

    errno = IoErr2errno(IoErr());
    return EOF;
} /* fflush */

