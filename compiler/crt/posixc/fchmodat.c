/*
    Copyright © 2025, The AROS Development Team.
    All rights reserved.

    POSIX.1-2008 function fchmodat
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

int fchmodat(int dirfd, const char *restrict pathname, mode_t mode, int flags)
{
#if (1)
    AROS_FUNCTION_NOT_IMPLEMENTED("posixc");
	return 0;
#else
    if (flags != 0) {
        // Only AT_SYMLINK_NOFOLLOW is valid, but chmod() cannot handle it portably
        errno = ENOTSUP;
        return -1;
    }

    if (dirfd == AT_FDCWD) {
        return chmod(pathname, mode);
    }

    int saved_cwd = open(".", O_RDONLY);
    if (saved_cwd < 0)
        return -1;

    if (fchdir(dirfd) != 0) {
        close(saved_cwd);
        return -1;
    }

    int result = chmod(pathname, mode);

    fchdir(saved_cwd);
    close(saved_cwd);
    return result;
#endif
}
