/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function close().
*/

#include "__arosc_privdata.h"

#include <unistd.h>
#include <stdlib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <errno.h>
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
        errno = EBADF;

        return -1;
    }

    if (--fdesc->fcb->opencount == 0)
    {
        /* Due to a *stupid* behaviour of the dos.library we cannot handle closing failures cleanly :-(
        if (
            !(fdesc->fcb->privflags & _FCB_DONTCLOSE_FH) &&
            !Close(fdesc->fh)
        )
        {
            fdesc->opencount++;
            errno = __arosc_ioerr2errno(IoErr());

            return -1;
        }
        */
        /* FIXME: Damn dos.library! We cannot report the error code correctly! This oughta change someday... */
        /* Since the dos.library destroys the file handle anyway, even if the closing fails, we cannot
           report the error code correctly, so just close the file and get out of here */

        if (!(fdesc->fcb->privflags & _FCB_DONTCLOSE_FH))
        {
            (void)Close(fdesc->fcb->fh);
        }

        FreeVec(fdesc->fcb);
    }

    __free_fdesc(fdesc);
    __setfdesc(fd, NULL);

    return 0;
} /* close */

