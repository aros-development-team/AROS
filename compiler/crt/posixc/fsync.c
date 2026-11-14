/*
    Copyright (C) 2004-2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function fsync().
*/

#include <exec/types.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <errno.h>
#include <fcntl.h>
#include "__fdesc.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

        int fsync(

/*  SYNOPSIS */
        int fd)

/*  FUNCTION
        Hands any data buffered in the file handle of fd to the file
        system.

    INPUTS
        fd - an open file descriptor.

    RESULT
        0 on success, -1 on error with errno set: EBADF when fd is not an
        open descriptor, EINVAL when fd belongs to another subsystem (a
        bsdsocket.library socket) and cannot be synchronised, or the error
        of the flush.

    NOTES
        A descriptor open only for reading, or one open on a directory, has
        nothing to write; fsync() returns 0 for it, as POSIX allows.

    EXAMPLE

    BUGS
        The file system's own caches are not written to the medium: when
        fsync() returns, the data has reached the file system, but it may
        not be on the device yet.

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    fdesc *fdesc = __getfdesc(fd);

    if (!fdesc)
    {
        /* A descriptor owned by another subsystem (a socket) is open, but
           cannot be synchronised. */
        errno = __getfdhooks(fd, NULL) ? EINVAL : EBADF;
        return -1;
    }

    /* A directory or a file not open for writing has no data to flush. */
    if ((fdesc->fcb->privflags & _FCB_ISDIR) || !(fdesc->fcb->flags & O_WRITE))
        return 0;

    __fcb_lock(fdesc->fcb);
    if (!Flush(fdesc->fcb->handle))
    {
        __fcb_unlock(fdesc->fcb);
        errno = __stdc_ioerr2errno(IoErr());
        return -1;
    }
    __fcb_unlock(fdesc->fcb);
    
    return 0;
} /* fsync */

