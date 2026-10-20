/*
    Copyright © 2025, The AROS Development Team.
    All rights reserved.

    POSIX.1-2008 function fstatvfs
*/

#include <aros/debug.h>
#include <errno.h>
#include <sys/statvfs.h>

#include "__fdesc.h"

/****************************************************************************
    NAME
        int fstatvfs(

    SYNOPSIS
        int fd, struct statvfs *buf)

    FUNCTION
        Check a file descriptor for a filesystem-statistics query.
        Retrieving the statistics is not implemented.

    INPUTS
        fd - Open posixc file descriptor.
        buf - Pointer to caller-owned struct statvfs storage.

    RESULT
        -1 with errno set to EINVAL if buf is NULL, or EBADF if
        fd is not recognized as a posixc file descriptor. The
        NULL-buffer check takes precedence. Otherwise returns 0
        without writing any part of *buf.

    NOTES
        Include <sys/statvfs.h>. The descriptor remains open.
        After validation, the function emits a not-implemented
        diagnostic through kprintf().

    BUGS
        A zero return does not indicate that filesystem statistics
        were obtained. Do not read an uninitialized buffer after
        this call. Use statvfs() with a suitable pathname instead.

    SEE ALSO
        statvfs()

*****************************************************************************/
int fstatvfs(int fd, struct statvfs *buf)
{
    if (!buf) {
        errno = EINVAL;
        return -1;
    }

    fdesc *desc = __getfdesc(fd);

    if (!desc)
    {
        errno = EBADF;

        return -1;
    }

#if (1)
    AROS_FUNCTION_NOT_IMPLEMENTED("posixc");
	return 0;
#else
    return statvfs("/", buf);
#endif
}
