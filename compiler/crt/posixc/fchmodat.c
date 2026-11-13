/*
    Copyright (C) 2025-2026, The AROS Development Team.
    All rights reserved.

    POSIX.1-2008 function fchmodat
*/

#include <aros/debug.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "__at.h"

/*****************************************************************************

    NAME */
#include <sys/stat.h>

        int fchmodat(

/*  SYNOPSIS */
        int dirfd,
        const char *restrict pathname,
        mode_t mode,
        int flags)

/*  FUNCTION
        Like chmod(), but a relative pathname is resolved against the
        directory referenced by dirfd instead of the current directory.

    INPUTS
        dirfd    - descriptor of an open directory, or AT_FDCWD
        pathname - file to change
        mode     - new permission bits
        flags    - 0 or AT_SYMLINK_NOFOLLOW (ignored: AROS has no lchmod())

    RESULT
        0 on success, -1 with errno set on failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        chmod(), openat()

    INTERNALS

******************************************************************************/
{
    BPTR oldcd, dirlock;
    int result;

    if (!pathname)
    {
        errno = EFAULT;
        return -1;
    }

    if (!__at_isabsolute(pathname) && __at_enter(dirfd, &oldcd, &dirlock) != 0)
        return -1;
    else if (__at_isabsolute(pathname))
        oldcd = dirlock = BNULL;

    result = chmod(pathname, mode);

    __at_leave(oldcd, dirlock);
    return result;
}
