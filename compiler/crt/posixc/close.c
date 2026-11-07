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

    /* A concurrent vfork copy pins entries through this same guard. Keep
       removal and destruction atomic with respect to that snapshot. */
    __lock_fdtable();
    if (!(fdesc = __getfdesc(fd)))
    {
        /* Not a local posixc file descriptor: dispatch to the owning
           subsystem's close hook (e.g. a bsdsocket socket).  The owner
           releases its own fd.library reservation. */
        APTR data;
        __unlock_fdtable();
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

    /* Unpublish under the snapshot guard, but do not hold the table across
       a potentially blocking final DOS close or shared-handle IO lock. */
    fcb *file = fdesc->fcb;
    __setfdesc(fd, NULL);
    __unlock_fdtable();
    __fcb_release(file);
    __free_fdesc(fdesc);

    return 0;
} /* close */

