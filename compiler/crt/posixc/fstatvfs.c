/*
    Copyright © 2025, The AROS Development Team.
    All rights reserved.

    POSIX.1-2008 function fstatvfs
*/

#include <aros/debug.h>
#include <errno.h>
#include <sys/statvfs.h>

#include "__fdesc.h"

int fstatvfs(int fd, struct statvfs *buf)
{
    if (!buf) {
        errno = EINVAL;
        return -1;
    }

    fdesc *desc = __getfdesc(fd);

    if (!desc)
    {
        errno = EBADF;

        return -1;
    }

#if (1)
    AROS_FUNCTION_NOT_IMPLEMENTED("posixc");
	return 0;
#else
    return statvfs("/", buf);
#endif
}
