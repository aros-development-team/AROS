/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function setvbuf()
    Lang: english
*/

#include <dos/stdio.h>
#include <proto/dos.h>
#include <errno.h>
#include "__open.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	int setvbuf (

/*  SYNOPSIS */
	FILE *stream,
	char *buf,
	int mode,
	size_t size)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    fdesc *desc;

    if (!stream)
    {
        GETUSER;

	errno = EFAULT;
	return EOF;
    }

    switch (mode)
    {
        case _IOFBF: mode = BUF_FULL; break;
	case _IOLBF: mode = BUF_LINE; break;
	case _IONBF: mode = BUF_NONE; break;
	default:
	{
	    GETUSER;

	    errno = EINVAL;
	    return EOF;
	}
    }

    desc = __getfdesc(stream->fd);
    if (!desc)
    {
	GETUSER;

	errno = EBADF;
	return EOF;
    }

    return SetVBuf(desc->fh, buf, mode, size ? size : -1);
} /* setvbuf */

