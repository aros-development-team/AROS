/*
    Copyright (C) 2025-2026, The AROS Development Team.
    All rights reserved.

    POSIX.1-2008 function unlinkat
*/

#include <aros/debug.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "__at.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

        int unlinkat(

/*  SYNOPSIS */
        int dirfd,
        const char *pathname,
        int flags)

/*  FUNCTION
        Like unlink() (or rmdir() when AT_REMOVEDIR is given), but a relative
        pathname is resolved against the directory referenced by dirfd
        instead of the current directory.

    INPUTS
        dirfd    - descriptor of an open directory, or AT_FDCWD
        pathname - entry to remove
        flags    - 0 or AT_REMOVEDIR

    RESULT
        0 on success, -1 with errno set on failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        unlink(), rmdir(), openat()

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

    /* unlink() is only an alias of remove() in the posixc linklib */
    result = (flags & AT_REMOVEDIR) ? rmdir(pathname) : remove(pathname);

    __at_leave(oldcd, dirlock);
    return result;
}
