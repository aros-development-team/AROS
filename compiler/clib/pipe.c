/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "__open.h"

static int clear_nonblock_flag (int desc)
{
    int oldflags = fcntl(desc, F_GETFL, 0);

    if (oldflags == -1)
	return -1;

    oldflags &= ~O_NONBLOCK;

    return fcntl(desc, F_SETFL, oldflags);
}

int pipe(int *pipedes)
{
    if (!pipedes)
    {
	errno = EFAULT;

	return -1;
    }

    pipedes[0] =  open("PIPEFS://unnamedpipe//", O_RDONLY|O_NONBLOCK);
    if (pipedes[0] != -1)
    {
	fdesc *desc = __getfdesc(pipedes[0]);
        BPTR olddir = CurrentDir(desc->fh);

	if
	(
	    clear_nonblock_flag(pipedes[0]) != -1 &&
       	    ((pipedes[1] = open("", O_WRONLY)) != -1)
	)
	{
      	    CurrentDir(olddir);

	    return 0;
	}

	CurrentDir(olddir);
	close(pipedes[0]);
    }

    return -1;
}
