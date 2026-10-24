/*
    Copyright (C) 2004-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <unistd.h>

#include <sys/ioctl.h>

#include <errno.h>

#include <libraries/fd.h>
#include <proto/dos.h>
#include <dos/dosextens.h>
#include <dos/consoleinfo.h>
#include <limits.h>
#include <string.h>
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
        TIOCGWINSZ returns zero on success. DOS errors include EBADF for an
        invalid descriptor, ENOTTY for unsupported queries, EFAULT for a NULL
        output, and EOVERFLOW for unrepresentable dimensions. Foreign hooks
        define their own return values and errors.

    NOTES
        TIOCGWINSZ uses a copied, versioned geometry reply from the console
        handler. It never dereferences handler-private InfoData fields.
        Older handlers and devices retain the ENOTTY fallback.
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
        Resize notifications and TIOCSWINSZ are not implemented.
        Failed queries leave the output structure unchanged.

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
        struct ConsoleGeometry geometry = {0};
        struct winsize result, *output;
        struct FileHandle *handle;
        va_list ap;

        /* A directory descriptor contains a FileLock, not a FileHandle. */
        if ((desc->fcb->privflags & _FCB_ISDIR) ||
            !IsInteractive(desc->fcb->handle))
        {
            errno = ENOTTY;
            return -1;
        }
        va_start(ap, request);
        output = va_arg(ap, struct winsize *);
        va_end(ap);
        if (!output)
        {
            errno = EFAULT;
            return -1;
        }
        handle = (struct FileHandle *)BADDR(desc->fcb->handle);
        if (!handle || !handle->fh_Type ||
            !DoPkt(handle->fh_Type, ACTION_CONSOLE_GETGEOMETRY,
                handle->fh_Arg1, (IPTR)&geometry, sizeof(geometry),
                CONSOLE_GEOMETRY_VERSION, 0))
        {
            errno = ENOTTY;
            return -1;
        }
        if (geometry.cg_Size != sizeof(geometry) ||
            geometry.cg_Version != CONSOLE_GEOMETRY_VERSION ||
            !geometry.cg_Columns || !geometry.cg_Rows)
        {
            errno = ENOTTY;
            return -1;
        }
        if (geometry.cg_Columns > USHRT_MAX || geometry.cg_Rows > USHRT_MAX ||
            geometry.cg_Width > USHRT_MAX || geometry.cg_Height > USHRT_MAX)
        {
            errno = EOVERFLOW;
            return -1;
        }
        memset(&result, 0, sizeof(result));
        result.ws_col = geometry.cg_Columns;
        result.ws_row = geometry.cg_Rows;
        result.ws_xpixel = geometry.cg_Width;
        result.ws_ypixel = geometry.cg_Height;
        *output = result;
        return 0;
    }
    errno = ENOTTY;
    return -1;
}
