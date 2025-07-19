/*
    Copyright © 2025, The AROS Development Team.
    All rights reserved.

    POSIX.1-2008 function fstatat
*/

#include <aros/debug.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

int unlinkat(int dirfd, const char *pathname, int flags)
{
#if (1)
    AROS_FUNCTION_NOT_IMPLEMENTED("posixc");
	return 0;
#else
    if (!pathname) {
        errno = EINVAL;
        return -1;
    }

    // Handle AT_FDCWD case directly
    if (dirfd == AT_FDCWD) {
        return (flags & AT_REMOVEDIR) ? rmdir(pathname) : unlink(pathname);
    }

    // Save current working directory
    int saved_cwd = open(".", O_RDONLY);
    if (saved_cwd < 0)
        return -1;

    // Change directory to dirfd
    if (fchdir(dirfd) != 0) {
        close(saved_cwd);
        return -1;
    }

    int result = (flags & AT_REMOVEDIR) ? rmdir(pathname) : unlink(pathname);

    // Restore working directory
    fchdir(saved_cwd);
    close(saved_cwd);

    return result;
#endif
}
