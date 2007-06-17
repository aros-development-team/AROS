/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "__errno.h"
#include "__open.h"

int pipe(int *pipedes)
{
    BPTR reader, writer;
    fdesc *rdesc, *wdesc;

    if (!pipedes)
    {
	errno = EFAULT;

	return -1;
    }

    if ((rdesc = malloc(sizeof(fdesc))) == NULL)
        return -1;
    if ((wdesc = malloc(sizeof(fdesc))) == NULL) {
        free(rdesc);
        return -1;
    }

    if (Pipe("PIPEFS:", &reader, &writer) != DOSTRUE) {
        errno = IoErr2errno(IoErr());
        free(rdesc);
        free(wdesc);
        return -1;
    }

    pipedes[0] = __getfirstfd(0);
    pipedes[1] = __getfirstfd(pipedes[0]);

    rdesc->fh        = reader;
    rdesc->flags     = O_RDONLY;
    rdesc->opencount = 1;
    __setfdesc(pipedes[0], rdesc);

    wdesc->fh        = writer;
    wdesc->flags     = O_WRONLY;
    wdesc->opencount = 1;
    __setfdesc(pipedes[1], wdesc);

    return 0;
}
