/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fopen().
*/

#include <fcntl.h>
#include <string.h>

#define POSIXC_NOSTDIO_DECL

#include "__stdio.h"
#include "__fdesc.h"

FILE * __fopen (
	const char * pathname,
	const char * mode,
	int    large)
{
    int fd;
    int openmode = __smode2oflags(mode);

    if (pathname && (strlen(pathname) > 0) && openmode != -1)
    {
	fdesc *fdesc;

        fd = open(pathname, openmode, 644);
        if (fd == -1)
            return NULL;

	fdesc = __getfdesc(fd);

	if (large)
	{
	    fdesc->fcb->privflags |= _FCB_FH64;
	}

        return fdopen(fd, NULL);
    }
    else
    {
        return NULL;
    }
} /* __fopen */
