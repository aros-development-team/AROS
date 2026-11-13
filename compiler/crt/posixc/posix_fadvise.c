/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <errno.h>

#include "__fdesc.h"

/*****************************************************************************

    NAME */
#include <fcntl.h>

        int posix_fadvise(

/*  SYNOPSIS */
        int fd,
        off_t offset,
        off_t len,
        int advice)

/*  FUNCTION
        Announce the access pattern the application intends to use on the
        region of the file starting at offset and spanning len bytes (or to
        the end of the file if len is 0), so that the system may optimise
        its caching accordingly.

    INPUTS
        fd     - An open file descriptor.
        offset - Start of the region the advice applies to.
        len    - Length of the region, 0 meaning "to end of file".
        advice - One of POSIX_FADV_NORMAL, POSIX_FADV_SEQUENTIAL,
                 POSIX_FADV_RANDOM, POSIX_FADV_NOREUSE, POSIX_FADV_WILLNEED
                 or POSIX_FADV_DONTNEED.

    RESULT
        0 on success, otherwise an error number (errno is not set):
        EBADF  - fd is not a valid file descriptor.
        EINVAL - advice is not a recognised value, offset or len is
                 negative, or fd refers to a directory.

    NOTES
        AROS filesystems provide no interface for caching hints, so the
        advice is validated and then ignored. This matches the behaviour
        permitted by POSIX: the call is purely advisory and has no effect on
        the semantics of subsequent operations on the file.

    EXAMPLE

    BUGS

    SEE ALSO
        open(), fcntl()

    INTERNALS

******************************************************************************/
{
    fdesc *desc = __getfdesc(fd);

    if (!desc)
    {
        /* Descriptors owned by another subsystem (e.g. sockets) are
           still valid descriptors; there is simply nothing to advise. */
        APTR data;
        if (__getfdhooks(fd, &data))
            return 0;
        return EBADF;
    }

    if (offset < 0 || len < 0)
        return EINVAL;

    if (desc->fcb->privflags & _FCB_ISDIR)
        return EINVAL;

    switch (advice)
    {
        case POSIX_FADV_NORMAL:
        case POSIX_FADV_SEQUENTIAL:
        case POSIX_FADV_RANDOM:
        case POSIX_FADV_NOREUSE:
        case POSIX_FADV_WILLNEED:
        case POSIX_FADV_DONTNEED:
            return 0;

        default:
            return EINVAL;
    }
} /* posix_fadvise */
