/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function vfprintf()
*/
/* Original source from libnix */


#include <proto/dos.h>
#include <errno.h>
#include <stdarg.h>
#include "__fdesc.h"
#include "__stdio.h"

static int __putc(int c, void *fh);

/*****************************************************************************

    NAME */
#include <stdio.h>

	int vfprintf (

/*  SYNOPSIS */
	FILE	   * stream,
	const char * format,
	va_list      args)

/*  FUNCTION
	Format a list of arguments and print them on the specified stream.

    INPUTS
	stream - A stream on which one can write
	format - A printf() format string.
	args - A list of arguments for the format string.

    RESULT
	The number of characters written.

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
	return 0;
    }

    return __vcformat ((void *)BADDR(fdesc->fcb->fh), __putc, format, args);
} /* vfprintf */


static int __putc(int c, void *fhp)
{
    BPTR fh = MKBADDR(fhp);
    if (FPutC(fh, c) == EOF)
    {
	errno = __arosc_ioerr2errno(IoErr());
	return EOF;
    }

    return c;
}
