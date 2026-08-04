/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function read().
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

        ssize_t read (

/*  SYNOPSIS */
        int    fd,
        void * buf,
        size_t count)

/*  FUNCTION
        Read an amount of bytes from a file descriptor.

    INPUTS
        fd - The file descriptor to read from
        buf - The buffer to read the bytes into
        count - Read this many bytes.

    RESULT
        The number of characters read (may range from 0 when the file
        descriptor contains no more characters to count) or -1 on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        open(), read(), fread()

    INTERNALS

******************************************************************************/
{
    ssize_t cnt;
    fdesc *fdesc = __getfdesc(fd);

    if (!fdesc)
    {
        /* Not a local posixc file descriptor: it may be a socket or other
           fd.library-owned descriptor.  Dispatch through its hooks. */
        APTR data;
        const struct fd_hooks *hooks = __getfdhooks(fd, &data);
        if (hooks && hooks->fdh_read)
        {
            LONG err = 0;
            cnt = (ssize_t)hooks->fdh_read(data, buf, count, &err);
            if (cnt < 0)
                errno = err;
            return cnt;
        }
        errno = EBADF;
        return -1;
    }

    if(fdesc->fcb->privflags & _FCB_ISDIR)
    {
        errno = EISDIR;
        return -1;
    }

    cnt = __dos64_read (fdesc->fcb, buf, count);

    if (cnt == -1)
        errno = __stdc_ioerr2errno (IoErr ());

    return cnt;
} /* read */

