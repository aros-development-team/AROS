/*
    Copyright © 2025, The AROS Development Team.
    All rights reserved.

    POSIX.1-2008 function fstatat
*/

#include <aros/debug.h>

#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>

int openat(int dirfd, const char *restrict pathname, int flags, ...)
{
#if (1)
    AROS_FUNCTION_NOT_IMPLEMENTED("posixc");
	return 0;
#else
    mode_t mode = 0;

    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, mode_t);
        va_end(ap);
    }

    if (dirfd == AT_FDCWD) {
        return open(pathname, flags, mode);
    }

    int saved_cwd = open(".", O_RDONLY);
    if (saved_cwd < 0)
        return -1;

    if (fchdir(dirfd) != 0) {
        close(saved_cwd);
        return -1;
    }

    int fd = open(pathname, flags, mode);

    fchdir(saved_cwd);
    close(saved_cwd);
    return fd;
#endif
}
