/*
    Copyright (C) 2004-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <unistd.h>

#include <sys/ioctl.h>

#include <errno.h>

#include <libraries/fd.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
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
        0 on success. -1 with errno set to EBADF for an invalid descriptor,
        EFAULT for a missing output structure, EOVERFLOW for a size that
        does not fit struct winsize, and ENOTTY for a request the descriptor
        does not support. Foreign hooks define their own return values and
        errors.

    NOTES
        TIOCGWINSZ asks the console handler with ACTION_CONSOLE_GEOMETRY,
        which reads nothing from and writes nothing to the console and
        leaves its mode alone. A descriptor whose handler does not answer
        the packet (a file, a pipe, an older console handler) gives ENOTTY.
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
        TIOCSWINSZ and window change notification are not implemented. On
        failure the output structure is left unchanged.

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    fdesc *desc = __getfdesc(fd);

    /* Descriptor owned by another subsystem (e.g. a bsdsocket socket):
       dispatch the ioctl (FIONBIO/FIONREAD/...) through its hook. */
    if (desc == NULL)
    {
        APTR data;
        const struct fd_hooks *hooks = __getfdhooks(fd, &data);
        if (hooks && hooks->fdh_ioctl)
        {
            va_list ap;
            APTR arg;
            LONG err = 0, r;
            va_start(ap, request);
            arg = va_arg(ap, APTR);
            va_end(ap);
            /* request is an int but the codes are unsigned 32-bit values
               with bit 31 set (FIONBIO = _IOW('f', 126, long) = 0x8008667e),
               so on 64-bit targets a plain (IPTR) cast sign-extends and no
               hook's switch can match it. Zero-extend instead. */
            r = hooks->fdh_ioctl(data, fd, (IPTR)(ULONG)request, arg, &err);
            if (r < 0)
                errno = err;
            return r;
        }
        errno = hooks ? ENOTTY : EBADF;
        return -1;
    }

    if (!desc->fcb)
    {
        errno = EBADF;
        return -1;
    }
    if (request == TIOCGWINSZ)
    {
        struct ConsoleGeometry cg = { 0 };
        struct FileHandle *h;
        struct winsize *ws;
        va_list ap;

        va_start(ap, request);
        ws = va_arg(ap, struct winsize *);
        va_end(ap);
        if (!ws)
        {
            errno = EFAULT;
            return -1;
        }
        /* A directory descriptor holds a FileLock, not a FileHandle */
        h = (desc->fcb->privflags & _FCB_ISDIR) ? NULL : BADDR(desc->fcb->handle);
        if (!h || !h->fh_Type ||
            DoPkt(h->fh_Type, ACTION_CONSOLE_GEOMETRY, (SIPTR)h->fh_Arg1, (SIPTR)&cg, sizeof(cg), 0, 0) != DOSTRUE)
        {
            errno = ENOTTY;
            return -1;
        }
        if (cg.cg_Columns > 0xFFFF || cg.cg_Rows > 0xFFFF || cg.cg_Width > 0xFFFF || cg.cg_Height > 0xFFFF)
        {
            errno = EOVERFLOW;
            return -1;
        }
        ws->ws_row    = cg.cg_Rows;
        ws->ws_col    = cg.cg_Columns;
        ws->ws_xpixel = cg.cg_Width;
        ws->ws_ypixel = cg.cg_Height;
        return 0;
    }
    errno = ENOTTY;
    return -1;
}
