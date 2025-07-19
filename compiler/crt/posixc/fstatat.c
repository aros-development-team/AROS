/*
    Copyright © 2025, The AROS Development Team.
    All rights reserved.

    POSIX.1-2008 function fstatat
*/

#include <aros/debug.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>

int fstatat(int dirfd, const char *restrict pathname, struct stat *restrict statbuf, int flags)
{
#if (1)
    AROS_FUNCTION_NOT_IMPLEMENTED("posixc");
	return 0;
#else
    if (!pathname || !statbuf) {
        errno = EINVAL;
        return -1;
    }

    if (dirfd == AT_FDCWD) {
        return (flags & AT_SYMLINK_NOFOLLOW) ? lstat(pathname, statbuf) : stat(pathname, statbuf);
    }

    int saved_cwd = open(".", O_RDONLY);
    if (saved_cwd < 0)
        return -1;

    if (fchdir(dirfd) != 0) {
        close(saved_cwd);
        return -1;
    }

    int result = (flags & AT_SYMLINK_NOFOLLOW) ? lstat(pathname, statbuf) : stat(pathname, statbuf);

    fchdir(saved_cwd);
    close(saved_cwd);
    return result;
#endif
}
