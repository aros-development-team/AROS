/*
    Copyright © 1995-2021, The AROS Development Team. All rights reserved.
    $Id$

    C99 function fopen().
*/

#include <aros/debug.h>

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
    int openmode;

    D(bug("[posixc] %s(0x%p, 0x%p, %u)\n", __func__, pathname,  mode, large));

    openmode = __smode2oflags(mode);

    D(bug("[posixc] %s: mode %x\n", __func__, openmode));
    
    if (pathname && (strlen(pathname) > 0) && openmode != -1)
    {
        FILE *ffile;
	fdesc *fdesc;

        D(bug("[posixc] %s: path '%s'\n", __func__, pathname));

        fd = open(pathname, openmode, 644);
        D(bug("[posixc] %s: fd = %x\n", __func__, fd));
        if (fd != -1)
        {
            fdesc = __getfdesc(fd);
            D(bug("[posixc] %s: fdesc @ %p\n", __func__, fdesc));
            if (large)
            {
                D(bug("[posixc] %s: FH64 set\n", __func__));
                fdesc->fcb->privflags |= _FCB_FH64;
            }

            ffile = fdopen(fd, NULL);

            D(bug("[posixc] %s: returning 0x%p\n", __func__, ffile));

            return ffile;
        }
    }

    D(bug("[posixc] %s: returning NULL\n", __func__));

    return NULL;
} /* __fopen */
