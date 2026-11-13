/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    C99 function fputs().
*/

#include <proto/dos.h>
#include <errno.h>
#include "__fdesc.h"
#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

        int fputs (

/*  SYNOPSIS */
        const char * str,
        FILE       * stream)

/*  FUNCTION
        Write a string to the specified stream.

    INPUTS
        str - Output this string...
        fh - ...to this stream

    RESULT
        > 0 on success and EOF on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        puts(), fputc(), putc()

    INTERNALS

******************************************************************************/
{
    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
        errno = EBADF;
        return EOF;
    }

    if (!str) str = "(null)";

    __fcb_lock(fdesc->fcb);
    if (FPuts(fdesc->fcb->handle, str) == -1)
    {
        errno = __stdc_ioerr2errno(IoErr());
        __fcb_unlock(fdesc->fcb);
        return EOF;
    }
    __fcb_unlock(fdesc->fcb);

    return 0;
} /* fputs */

