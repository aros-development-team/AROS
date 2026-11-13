/*
    Copyright (C) 2025-2026, The AROS Development Team.
    All rights reserved.

    POSIX.1-2008 function openat
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

#include "__at.h"

/*****************************************************************************

    NAME */
#include <fcntl.h>

        int openat(

/*  SYNOPSIS */
        int dirfd,
        const char *restrict pathname,
        int flags,
        ...)

/*  FUNCTION
        Like open(), but a relative pathname is resolved against the
        directory referenced by the descriptor dirfd instead of the current
        directory. dirfd may be AT_FDCWD to use the current directory.

    INPUTS
        dirfd    - descriptor of an open directory (see opendir(), dirfd(),
                   or open() with O_DIRECTORY), or AT_FDCWD
        pathname - file to open; absolute names ("Volume:...", "/...") ignore dirfd
        flags    - as for open(); O_DIRECTORY requires the target to be a
                   directory, O_CLOEXEC sets FD_CLOEXEC on the new descriptor
        mode     - permissions used when O_CREAT creates the file

    RESULT
        The new file descriptor, or -1 with errno set.

    NOTES
        The directory is made current with dos.library/CurrentDir() only for
        the duration of the call.

    EXAMPLE

    BUGS

    SEE ALSO
        open(), fdopendir(), fstatat(), unlinkat()

    INTERNALS

******************************************************************************/
{
    mode_t mode = 0;
    BPTR oldcd, dirlock;
    int fd;

    if (flags & O_CREAT)
    {
        va_list ap;
        va_start(ap, flags);
        mode = (mode_t)va_arg(ap, int);
        va_end(ap);
    }

    if (!pathname)
    {
        errno = EFAULT;
        return -1;
    }

    if (__at_isabsolute(pathname))
        return open(pathname, flags, mode);

    if (__at_enter(dirfd, &oldcd, &dirlock) != 0)
        return -1;

    fd = open(pathname, flags, mode);

    __at_leave(oldcd, dirlock);

    D(bug("[posixc] %s(%d, \"%s\", 0x%x) = %d\n", __func__, dirfd, pathname, flags, fd));
    return fd;
}
