#ifndef _SYS_UIO_H_
#define _SYS_UIO_H_
/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    POSIX/BSD header <sys/uio.h>
*/

#include <aros/system.h>

#include <aros/types/iovec_s.h>
#include <aros/types/ssize_t.h>

#if __BSD_VISIBILE
enum uio_rw
{
    UIO_READ,
    UIO_WRITE
};

enum uio_seg
{
    UIO_USERSPACE,
    UIO_SYSSPACE,
    UIO_NOCOPY
};

/* XXX This really doesn't belong here... */
struct Task;

struct uio
{
    struct iovec        *uio_iov;
    int                  uio_iovcnt;
    off_t                uio_offset;
    int                  uio_resid;
    enum uio_seg         uio_segflg;
    enum uio_rw          uio_rw;
    struct Task         *uio_task;
};
#endif /* __BSD_VISIBLE */

__BEGIN_DECLS
ssize_t readv(int fd, const struct iovec *iovec, int count);
ssize_t writev(int fd, const struct iovec *iovec, int count);
__END_DECLS

#endif /* _SYS_UIO_H_ */
