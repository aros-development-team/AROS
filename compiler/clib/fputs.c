/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function fputs()
    Lang: english
*/

#include <proto/dos.h>
#include <errno.h>
#include "__errno.h"
#include "__open.h"

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

    HISTORY
	10.12.1996 digulla created

******************************************************************************/
{
    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
    	errno = EBADF;
	return EOF;
    }

    if (!str) str = "(null)";

    if (FPuts((BPTR)fdesc->fh, str) == -1)
    {
	errno = IoErr2errno(IoErr());
	return EOF;
    }

    return 0;
} /* fputs */

