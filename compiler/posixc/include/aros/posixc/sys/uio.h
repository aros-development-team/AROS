#ifndef _SYS_UIO_H_
#define _SYS_UIO_H_
/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: POSIX.1-2008 header file <sys/uio.h>
*/

#include <aros/system.h>

#include <aros/types/iovec_s.h>
#include <aros/types/ssize_t.h>

__BEGIN_DECLS

ssize_t readv(int fd, const struct iovec *iovec, int count);
ssize_t writev(int fd, const struct iovec *iovec, int count);

__END_DECLS

#endif /* _SYS_UIO_H_ */
