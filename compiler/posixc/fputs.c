/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fputs().
*/

#include <proto/dos.h>
#include <errno.h>
#include "__fdesc.h"
#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

	int fputs (

/*  SYNOPSIS */
	const char * str,
	FILE	   * stream)

/*  FUNCTION
	Write a string to the specified stream.

    INPUTS
	str - Output this string...
	fh - ...to this stream

    RESULT
	> 0 on success and EOF on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	puts(), fputc(), putc()

    INTERNALS

******************************************************************************/
{
    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
    	errno = EBADF;
	return EOF;
    }

    if (!str) str = "(null)";

    if (FPuts((BPTR)fdesc->fcb->fh, str) == -1)
    {
	errno = __stdc_ioerr2errno(IoErr());
	return EOF;
    }

    return 0;
} /* fputs */

