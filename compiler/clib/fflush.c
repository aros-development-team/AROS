/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI C function fflush()
    Lang: english
*/
#include <errno.h>
#include <exec/types.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>

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

    switch ((IPTR)stream)
    {
    case 1:
	fh = Input ();

	break;

    case 0: /* TODO flush all output streams */
    case 2:
	fh = Output ();

	break;

    case 3: {
	struct Process *me=(struct Process *)FindTask(NULL);

	fh = me->pr_CES ? me->pr_CES : me->pr_COS;

	break; }

    default:
	fh = (BPTR)stream->fh;
	break;
    }

    if (fh && Flush (fh))
	return 0;

    if (!fh)
	errno = EINVAL;
    else
    {
	switch (IoErr())
	{
	case ERROR_OBJECT_WRONG_TYPE:
	    errno = EINVAL;
	    break;

	case ERROR_NO_FREE_STORE:
	    errno = ENOMEM;
	    break;

	case ERROR_OBJECT_NOT_FOUND:
	    errno = ENOENT;
	    break;

	default:
	    errno = ENOSYS;
	    break;
	}
    }

    return EOF;
} /* fflush */

