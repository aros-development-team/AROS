/*
    Copyright (C) 2004-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <unistd.h>

#include <sys/ioctl.h>

#include <errno.h>

#include <libraries/fd.h>
#include "__fdesc.h"

/*****************************************************************************

    NAME */

#include <sys/ioctl.h>

        int ioctl(

/*  SYNOPSIS */
        int fd,
        int request,
        ...)

/*  FUNCTION
        Control device. Function to manipulate and fetch special device
        parameters.

    INPUTS
        fd      - file descriptor
        request - ioctl request id, containing request type, input or output
                  type and argument size in bytes. Use macros and defines
                  from <sys/ioctl.h>:

                  TIOCGWINSZ - fill in rows, columns, width and height of
                               console window

        ...     - Other arguments for the specified request

    RESULT
        DOS handles return -1 with errno set to EBADF (invalid descriptor)
        or ENOTTY (unsupported operation). Foreign hooks define their own
        return values and errors.

    NOTES
        DOS terminal geometry is currently unsupported. ACTION_DISK_INFO
        does not provide a validated geometry contract: mainline CON stores
        a use count in id_InUse, not a console.device IO request pointer.
        See rom/filesys/console_handler/con_handler.c, ACTION_DISK_INFO:
        id->id_InUse = fh->usecount;
        Foreign descriptor hooks retain their own request handling.

    EXAMPLE
        #include <stdio.h>
        #include <unistd.h>
        #include <sys/ioctl.h>

        {
            int ret;
            struct winsize w;
            ret = ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
            if(ret)
            {
                printf("ERROR: %d\n", ret);
            }
            else
            {
                printf ("columns: %4d\n", w.ws_col);
                printf ("lines:   %4d\n", w.ws_row);
                printf ("width:   %4d\n", w.ws_xpixel);
                printf ("height:  %4d\n", w.ws_ypixel);
            }
        }

    BUGS
        TIOCGWINSZ on DOS handles returns ENOTTY until a terminal-control
        protocol is available. The output structure is left unchanged.

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    fdesc *desc = __getfdesc(fd);

    /* Descriptor owned by another subsystem (e.g. a bsdsocket socket):
       dispatch the ioctl (FIONBIO/FIONREAD/...) through its hook. */
    if (desc == NULL)
    {
        LONG err = 0;
        va_list ap;
        APTR arg;
        SIPTR result;

        va_start(ap, request);
        arg = va_arg(ap, APTR);
        va_end(ap);
        result = __fdhook(fd, __FDH_IOCTL, (IPTR)(ULONG)request,
                          (IPTR)arg, 0, &err);
        if (result < 0)
            errno = err;
        return result;
    }

    if (!desc->fcb)
    {
        errno = EBADF;
        return -1;
    }
    /* Never reinterpret handler-private InfoData fields as pointers.
       No DOS request is supported here yet, so do not consume its varargs
       or inspect its handle (which may be a directory FileLock). */
    errno = ENOTTY;
    return -1;
}
