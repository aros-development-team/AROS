/*
    Copyright (C) 2026 John KNIPPER <code@jkn.me>.
    Licensed under the AROS Public License (APL), Version 1.1.

    Positioned write over the native DOS handle. The shared fcb semaphore
    spans the entire operation; no seek-and-restore sequence is exposed to
    other POSIX/stdio users. File-handler offset limitations are reported
    directly, without growing a gap or changing flags to imitate success.
*/
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <proto/dos.h>
#include "__fdesc.h"
#include "__dos64.h"

ssize_t pwrite64(int d, const void *buf, size_t nbytes, __off64_t offset)
{
    fdesc *fdesc = __getfdesc(d);
    QUAD old, count = -1;
    int error = 0;
    if (!fdesc) { errno = EBADF; return -1; }
    if (offset < 0) { errno = EINVAL; return -1; }
    if (nbytes && !buf) { errno = EFAULT; return -1; }
    __fcb_lock(fdesc->fcb);
    if ((fdesc->fcb->flags & O_ACCMODE) == O_RDONLY) {
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
    if (__dos64_seek(fdesc->fcb, offset, OFFSET_BEGINNING) < 0) {
        error = __stdc_ioerr2errno(IoErr());
        goto restore;
    }
    if (nbytes > ((size_t)-1 >> 1))
        nbytes = (size_t)-1 >> 1;
    count = __dos64_write(fdesc->fcb, buf, nbytes);
    if (count < 0)
        error = __stdc_ioerr2errno(IoErr());
restore:
    if (__dos64_seek(fdesc->fcb, old, OFFSET_BEGINNING) < 0 && !error)
        error = __stdc_ioerr2errno(IoErr());
done:
    __fcb_unlock(fdesc->fcb);
    if (error) { errno = error; return -1; }
    return (ssize_t)count;
}
