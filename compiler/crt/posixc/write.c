/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function write().
*/

#include <errno.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <libraries/fd.h>
#include <aros/debug.h>
#include "__fdesc.h"
#include "__dos64.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

        ssize_t write (

/*  SYNOPSIS */
        int          fd,
        const void * buf,
        size_t       count)

/*  FUNCTION
        Write an amount of characters to the specified file descriptor.

    INPUTS
        fd - The file descriptor to write to
        buf - Write these bytes into the file descriptor
        count - Write that many bytes

    RESULT
        The number of characters written or -1 on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    ssize_t cnt;

    fdesc *fdesc = __getfdesc(fd);
    if (!fdesc)
    {
        /* Not a local posixc file descriptor: dispatch to the owning
           subsystem's hooks (e.g. a bsdsocket socket). */
        APTR data;
        const struct fd_hooks *hooks = __getfdhooks(fd, &data);
        if (hooks && hooks->fdh_write)
        {
            LONG err = 0;
            cnt = (ssize_t)hooks->fdh_write(data, buf, count, &err);
            if (cnt < 0)
                errno = err;
            return cnt;
        }
        errno = EBADF;
        return -1;
    }

    cnt = __dos64_write (fdesc->fcb, (void *)buf, count);

    if (cnt == -1)
        errno = __stdc_ioerr2errno (IoErr ());

    return cnt;
} /* write */

