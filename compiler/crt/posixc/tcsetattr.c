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
        Currently supports only ICANON. For compatibility, TCSANOW,
        TCSADRAIN and TCSAFLUSH all apply the mode immediately. Output drain
        and input flush are not implemented; accepting these actions does
        not provide their full POSIX semantics.

    EXAMPLE

    BUGS

    SEE ALSO
        ioctl()

    INTERNALS

******************************************************************************/
{
    fdesc *fdesc = __getfdesc(fd);

    if (!fdesc || !fdesc->fcb)
    {
        errno = EBADF;
        return -1;
    }

    if (!t)
    {
        errno = EFAULT;
        return -1;
    }
    if (opt != TCSANOW && opt != TCSADRAIN && opt != TCSAFLUSH)
    {
        errno = EINVAL;
        return -1;
    }
    if ((fdesc->fcb->privflags & _FCB_ISDIR) ||
        !IsInteractive(fdesc->fcb->handle))
    {
        errno = ENOTTY;
        return -1;
    }
    if (!SetMode(fdesc->fcb->handle, (t->c_lflag & ICANON) ? 0 : 1))
    {
        errno = __stdc_ioerr2errno(IoErr());
        if (!errno) errno = EIO;
        return -1;
    }
    /* Do not claim a mode transition that the handler rejected. */
    if (t->c_lflag & ICANON)
        fdesc->fcb->privflags &= ~_FCB_CONSOLERAW;
    else
        fdesc->fcb->privflags |= _FCB_CONSOLERAW;
    return 0;
}
