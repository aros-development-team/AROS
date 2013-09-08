/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fflush().
*/
#include <exec/lists.h>
#include <proto/dos.h>
#include <errno.h>

#include "__stdio.h"
#include "__stdcio_intbase.h"

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
    if (stream == NULL)
    {
        /* flush all streams opened for output */
        struct StdCIOIntBase *StdCIOBase =
            (struct StdCIOIntBase *)__aros_getbase_StdCIOBase();
	FILE *f;

	ForeachNode (&StdCIOBase->files, f)
	{
            if (fflush(f) == EOF)
                return EOF;
        }
    }
    else
    {
	if (!Flush(stream->fh))
        {
            errno = __stdc_ioerr2errno(IoErr());
            return EOF;
        }
    }

    return 0;
} /* fflush */
