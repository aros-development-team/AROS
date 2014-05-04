/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function vfscanf()
*/

/* Original source from libnix */

#include <proto/dos.h>
#include <errno.h>
#include <stdarg.h>
#include "__fdesc.h"
#include "__stdio.h"

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

static int __getc(void *_h);
static int __ungetc(int c, void *_h);

#endif

/*****************************************************************************

    NAME */
#include <stdio.h>

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

******************************************************************************/
{
#if VFSCANF_DIRECT_DOS
    struct __vfscanf_handle h;
    
    h.stream = stream;
    h.fdesc  = __getfdesc(stream->fd);

    if (!h.fdesc)
    {
	errno = EBADF;
	return 0;
    }


    Flush (h.fdesc->fcb->handle);

    return __vcscan (&h, __getc, __ungetc, format, args);
#else
    fdesc *fdesc;
    
    fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
	errno = EBADF;
	return 0;
    }

    Flush (fdesc->fcb->handle);
    
    return __vcscan (stream, fgetc, ungetc, format, args);
       
#endif
} /* vfscanf */

#if VFSCANF_DIRECT_DOS

static int __ungetc(int c, void *_h)
{
    struct __vfscanf_handle *h = _h;
    /* Note: changes here might require changes in ungetc.c!! */

    if (c < -1)
	c = (unsigned int)c;

    if (!UnGetC(h->fdesc->fcb->handle, c))
    {
	errno = __stdc_ioerr2errno(IoErr());

	if (errno)
	{
	    h->stream->flags |= __POSIXC_STDIO_ERROR;
	}
	else
	{
	    h->stream->flags |= __POSIXC_STDIO_EOF;
	}
	
	c = EOF;
    }

    return c;
}

static int __getc(void *_h)
{
    struct __vfscanf_handle *h = _h;
    fdesc *fdesc = h->fdesc;
    int c;

    /* Note: changes here might require changes in fgetc.c!! */

    FLUSHONREADCHECK

    c = FGetC(h->fdesc->fcb->handle);
    
    if (c == EOF)
    {
    	c = IoErr();
	
	if (c)
	{
	    errno = __stdc_ioerr2errno(c);
	    h->stream->flags |= __POSIXC_STDIO_ERROR;
	}
	else
	{
	    h->stream->flags |= __POSIXC_STDIO_EOF;
	}
	
	c = EOF;
    }

    return c;
}

#endif
