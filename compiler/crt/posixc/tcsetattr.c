/*
    Copyright (C) 1995-2016, The AROS Development Team. All rights reserved.
*/

#include <proto/dos.h>

#include "__fdesc.h"

/*****************************************************************************

    NAME */

#include <sys/ioctl.h>
#include <termios.h>
#include <errno.h>

    int tcsetattr(

/*  SYNOPSIS */
        int fd,
        int opt,
        const struct termios *t)

/*  FUNCTION
        Set terminal attributes.

    INPUTS
        fd      - file descriptor
        opt     - optional actions
        t       - struct termios containing the requested changes

    RESULT
         0      - success
        -1      - error

    NOTES
        Will return success, if *any* of the changes were successful.
        Currently supports only ICANON flag

    EXAMPLE

    BUGS

    SEE ALSO
        ioctl()

    INTERNALS

******************************************************************************/
{
    fdesc *fdesc = __getfdesc(fd);

    if (!fdesc)
    {
        errno = EBADF;
        return -1;
    }

    if (IsInteractive(fdesc->fcb->handle))
    {
        if (t->c_lflag & ICANON)
        {
            SetMode(fdesc->fcb->handle, 0);
            fdesc->fcb->privflags &= ~_FCB_CONSOLERAW;
        }
        else
        {
            SetMode(fdesc->fcb->handle, 1);
            fdesc->fcb->privflags |= _FCB_CONSOLERAW;
        }
    }

    return 0;
}
