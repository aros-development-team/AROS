/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fflush().
*/

#include "__posixc_intbase.h"

#include <exec/types.h>
#include <exec/lists.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <errno.h>
#include "__stdio.h"
#include "__fdesc.h"

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
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    /* flush all streams opened for output */
    if (!stream)
    {
	FILENODE *fn;

	ForeachNode (&PosixCBase->stdio_files, fn)
	{
	    if (fn->File.flags & __POSIXC_STDIO_WRITE)
	    {
	    	fdesc *fdesc = __getfdesc(fn->File.fd);

		if (!fdesc)
		{
		    errno = EBADF;
		    return EOF;
      		}

		if (!Flush(fdesc->fcb->handle))
		{
		    errno = __stdc_ioerr2errno(IoErr());
		    return EOF;
      		}
            }
        }
    }
    else
    {
    	fdesc *fdesc = __getfdesc(stream->fd);

	if (!fdesc || !(stream->flags & __POSIXC_STDIO_WRITE))
	{
	    errno = EBADF;
	    return EOF;
	}

	if (Flush(fdesc->fcb->handle))
	    return 0;
    }

    errno = __stdc_ioerr2errno(IoErr());
    return EOF;
} /* fflush */

