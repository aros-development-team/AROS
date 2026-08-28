/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function close().
*/

#include <unistd.h>
#include <stdlib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <errno.h>
#include <libraries/fd.h>
#include "__fdesc.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

        int close (

/*  SYNOPSIS */
        int fd)

/*  FUNCTION
        Closes an open file. If this is the last file descriptor
        associated with this file, then all allocated resources
        are freed, too.

    INPUTS
        fd - The result of a successful open()

    RESULT
        -1 for error or zero on success.

    NOTES
        This function must not be used in a shared library or
        in a threaded application.

    EXAMPLE

    BUGS

    SEE ALSO
        open(), read(), write(), fopen()

    INTERNALS

******************************************************************************/
{
    fdesc *fdesc;

    if (!(fdesc = __getfdesc(fd)))
    {
        /* Not a local posixc file descriptor: dispatch to the owning
           subsystem's close hook (e.g. a bsdsocket socket).  The owner
           releases its own fd.library reservation. */
        APTR data;
        const struct fd_hooks *hooks = __getfdhooks(fd, &data);
        if (hooks && hooks->fdh_close)
        {
            LONG err = 0;
            if (hooks->fdh_close(data, fd, &err) < 0)
            {
                errno = err;
                return -1;
            }
            return 0;
        }
        errno = EBADF;

        return -1;
    }

    /* Serialise the reference-count decrement and the final teardown
       against other Tasks sharing this fcb. When the count reaches zero
       no other Task holds a reference, so releasing the lock immediately
       before FreeVec() is safe. */
    __fcb_lock(fdesc->fcb);
    if (--fdesc->fcb->opencount == 0)
    {
        /* Due to a *stupid* behaviour of the dos.library we cannot handle closing failures cleanly :-(
        if (
            !(fdesc->fcb->privflags & _FCB_DONTCLOSE_FH) &&
            !Close(fdesc->fh)
        )
        {
            fdesc->opencount++;
            errno = __stdc_ioerr2errno(IoErr());

            return -1;
        }
        */
        /* FIXME: Damn dos.library! We cannot report the error code correctly! This oughta change someday... */
        /* Since the dos.library destroys the file handle anyway, even if the closing fails, we cannot
           report the error code correctly, so just close the file and get out of here */

        if (!(fdesc->fcb->privflags & _FCB_DONTCLOSE_FH))
        {
            // don't close directories because we don't Open() them.
            if (fdesc->fcb->privflags & _FCB_ISDIR)
            {
                UnLock(fdesc->fcb->handle);
            }
            else
            {
                Close(fdesc->fcb->handle);
            }
        }

        __fcb_unlock(fdesc->fcb);
        FreeVec(fdesc->fcb);
    }
    else
        __fcb_unlock(fdesc->fcb);

    __free_fdesc(fdesc);
    __setfdesc(fd, NULL);

    return 0;
} /* close */

