/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
#include <stdio.h>

/*
** If the VFSCANF_DIRECT_DOS define is set to 1, dos.library functions FGetC()
** and UnGetC() are used directly, for possibly better speed. Otherwise
** the clib functions fgetc/ungetc are used.
*/
 
#define VFSCANF_DIRECT_DOS 1

#if VFSCANF_DIRECT_DOS

struct __vfscanf_handle
{
    FILE  *stream;
    fdesc *fdesc;
};

static int __getc(struct __vfscanf_handle *h);
static int __ungetc(int c, struct __vfscanf_handle *h);

#endif

/*****************************************************************************

    NAME */
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
#if VFSCANF_DIRECT_DOS
    struct __vfscanf_handle h;
    
    h.stream = stream;
    h.fdesc  = __getfdesc(stream->fd);

    if (!h.fdesc)
    {
        GETUSER;

	errno = EBADF;
	return 0;
    }


    Flush (h.fdesc->fh);

    return __vcscan (&h, __getc, __ungetc, format, args);
#else
    fdesc *fdesc;
    
    fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
        GETUSER;

	errno = EBADF;
	return 0;
    }

    Flush (fdesc->fh);
    
    return __vcscan (stream, fgetc, ungetc, format, args);
       
#endif
} /* vfscanf */

#if VFSCANF_DIRECT_DOS

static int __ungetc(int c, struct __vfscanf_handle *h)
{
    /* Note: changes here might require changes in ungetc.c!! */

    if (c < -1)
	c = (unsigned int)c;

    if (!UnGetC((BPTR)h->fdesc->fh, c))
    {
    	GETUSER;

	errno = IoErr2errno(IoErr());

	if (errno)
	{
	    h->stream->flags |= _STDIO_ERROR;
	}
	else
	{
	    h->stream->flags |= _STDIO_EOF;
	}
	
	c = EOF;
    }

    return c;
}

static int __getc(struct __vfscanf_handle *h)
{
    int c;

    /* Note: changes here might require changes in fgetc.c!! */

    c = FGetC((BPTR)h->fdesc->fh);
    
    if (c == EOF)
    {
    	GETUSER;

    	c = IoErr();
	
	if (c)
	{
	    errno = IoErr2errno(c);
	    h->stream->flags |= _STDIO_ERROR;
	}
	else
	{
	    h->stream->flags |= _STDIO_EOF;
	}
	
	c = EOF;
    }

    return c;
}

#endif
