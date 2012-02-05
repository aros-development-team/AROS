/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function setvbuf().
*/

#include <dos/stdio.h>
#include <proto/dos.h>
#include <errno.h>
#include "__fdesc.h"
#include "__stdio.h"

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

******************************************************************************/
{
    fdesc *desc;

    if (!stream)
    {
	errno = EFAULT;
	return EOF;
    }

    /* Fail if provided buffer is smaller than minimum required by DOS */
    if (buf && size < 208)
    {
	errno = EFAULT;
	return EOF;
    }

    switch (mode)
    {
        case _IOFBF: mode = BUF_FULL; break;
	case _IOLBF: mode = BUF_LINE; break;
	case _IONBF: mode = BUF_NONE; break;
	default:
            errno = EINVAL;
	    return EOF;
    }

    desc = __getfdesc(stream->fd);
    if (!desc)
    {
	errno = EBADF;
	return EOF;
    }

    return SetVBuf(desc->fcb->fh, buf, mode, size ? size : -1);
} /* setvbuf */

