/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function fstat()
    Lang: English
*/

#include <errno.h>

#include "__stat.h"
#include "__open.h"

#include <sys/stat.h>


int fstat(int fd, struct stat *sb)
{
    fdesc *desc = __getfdesc(fd);

    if (!desc)
    {
        errno = EBADF;

	return -1;
    }

    return __stat(desc->fh, sb);
}
