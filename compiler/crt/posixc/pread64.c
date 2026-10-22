/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Copyright (C) 2026 John KNIPPER <code@jkn.me>.
    Licensed under the AROS Public License (APL), Version 1.1.

    Read from a file at a given offset.
*/
#include <errno.h>
#include <fcntl.h>
#include <proto/dos.h>
#include "__fdesc.h"
#include "__dos64.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

        ssize_t pread64 (

/*  SYNOPSIS */
        int        d,
        void      *buf,
        size_t     nbytes,
        __off64_t  offset)

/*  FUNCTION
        Read up to nbytes from the file at offset, leaving the file offset
        of the descriptor unchanged.

    INPUTS
        d      - file descriptor to read from
        buf    - where to put the data
        nbytes - number of bytes to read
        offset - position in the file to read from

    RESULT
        The number of bytes read, 0 at or beyond the end of the file, or -1
        with errno set.

    NOTES
        The fcb semaphore spans position capture, seek, read and restoration.
        Other POSIX/stdio users of duplicated descriptors use the same lock,
        so they cannot observe the temporary position. Reads at or beyond EOF
        do not seek or extend the file. Restoration uses the DOS primitive,
        which cannot silently grow the file like lseek64()'s fallback.

    EXAMPLE

    BUGS

    SEE ALSO
        pread(), read(), lseek64()

    INTERNALS

******************************************************************************/
{
    fdesc *fdesc = __getfdesc(d);
    QUAD old, size, count = -1;
    int error = 0;

    if (!fdesc) { errno = EBADF; return -1; }
    if (offset < 0) { errno = EINVAL; return -1; }
    if (nbytes && !buf) { errno = EFAULT; return -1; }

    __fcb_lock(fdesc->fcb);
    if ((fdesc->fcb->flags & O_ACCMODE) == O_WRONLY) {
        error = EBADF;
        goto done;
    }
    if (fdesc->fcb->privflags & _FCB_ISDIR) {
        error = EISDIR;
        goto done;
    }
    FLUSHONREADCHECK
    old = __dos64_getpos(fdesc->fcb);
    if (old < 0) {
        error = __stdc_ioerr2errno(IoErr());
        goto done;
    }
    if (!nbytes) { count = 0; goto done; }
    size = __dos64_getsize(fdesc->fcb);
    if (size < 0) {
        error = __stdc_ioerr2errno(IoErr());
        goto restore;
    }
    if (offset >= size) { count = 0; goto done; }
    if (__dos64_seek(fdesc->fcb, offset, OFFSET_BEGINNING) < 0) {
        error = __stdc_ioerr2errno(IoErr());
        goto restore;
    }
    if (nbytes > ((size_t)-1 >> 1))
        nbytes = (size_t)-1 >> 1;
    count = __dos64_read(fdesc->fcb, buf, nbytes);
    if (count < 0)
        error = __stdc_ioerr2errno(IoErr());
restore:
    /* A failed restoration is an error even after a successful transfer.
       Preserve an earlier I/O error when both operations fail. */
    if (__dos64_seek(fdesc->fcb, old, OFFSET_BEGINNING) < 0 && !error)
        error = __stdc_ioerr2errno(IoErr());
done:
    __fcb_unlock(fdesc->fcb);
    if (error) { errno = error; return -1; }
    return (ssize_t)count;
} /* pread64 */
