/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Scan a stream and write the result in the parameters
    Lang: english
*/
/* Original source from libnix */
#define AROS_ALMOST_COMPATIBLE

#include <proto/dos.h>
#include <errno.h>
#include "__errno.h"
#include "__open.h"

static int __getc(BPTR fh);
static int __ungetc(int c, BPTR fh);

/*****************************************************************************

    NAME */
#include <stdio.h>
#include <stdarg.h>

	int vfscanf (

/*  SYNOPSIS */
	FILE	   * stream,
	const char * format,
	va_list      args)

/*  FUNCTION
	Read the scream, scan it as the format specified and write the
	result of the conversion into the specified arguments.

    INPUTS
	stream - A stream to read from
	format - A scanf() format string.
	args - A list of arguments for the results.

    RESULT
	The number of converted arguments.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	28.01.1997 digulla created

******************************************************************************/
{
    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
        GETUSER;

	errno = EBADF;
	return 0;
    }


    Flush (fdesc->fh);

    return __vcscan (fdesc->fh, __getc, __ungetc, format, args);
} /* vfscanf */

static int __ungetc(int c, BPTR fh)
{
    if (!UnGetC(fh, c))
    {
    	GETUSER;

	errno = IoErr2errno(IoErr());
	return EOF;
    }

    return c;
}

static int __getc(BPTR fh)
{
    int c;

    if ((c = FGetC(fh)) == -1)
    {
    	GETUSER;

	errno = IoErr2errno(IoErr());
	return EOF;
    }

    return c;
}
