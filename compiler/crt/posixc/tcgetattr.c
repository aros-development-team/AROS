/*
    Copyright (C) 2016, The AROS Development Team. All rights reserved.
*/

#include <proto/dos.h>

#include "__fdesc.h"

/*****************************************************************************

    NAME */

#include <sys/ioctl.h>
#include <termios.h>
#include <errno.h>

    int tcgetattr(

/*  SYNOPSIS */
        int fd,
        struct termios *t)

/*  FUNCTION
        Get terminal attributes.

    INPUTS
        fd      - file descriptor
        t       - struct termios where attributes are put

    RESULT
         0      - success
        -1      - error

    NOTES
        Currently supports only ICANON flag

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
    /* Directory descriptors contain a FileLock, not a DOS FileHandle. */
    if ((fdesc->fcb->privflags & _FCB_ISDIR) ||
        !IsInteractive(fdesc->fcb->handle))
    {
        errno = ENOTTY;
        return -1;
    }

    if (fdesc->fcb->privflags & _FCB_CONSOLERAW)
        t->c_lflag &= ~ICANON;
    else
        t->c_lflag |= ICANON;

    return 0;
}
