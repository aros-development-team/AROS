/*
    Copyright (C) 1995-2021, The AROS Development Team. All rights reserved.

    C99 function fgetc().
*/

#include <aros/debug.h>

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <errno.h>

#include "__fdesc.h"
#include "__stdio.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

/*      int fgetc (

    SYNOPSIS
        FILE * stream

    FUNCTION
        Read one character from the stream. If there is no character
        available or an error occurred, the function returns EOF.

    INPUTS
        stream - Read from this stream

    RESULT
        The character read or EOF on end of file or error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        getc(), __posixc_fputc(), putc()

    INTERNALS

******************************************************************************/
int  __posixc_fgetc (   FILE * stream)
{
    int c;
    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
        errno = EBADF;
        stream->flags |= __POSIXC_STDIO_ERROR;
        return EOF;
    }

    /* Note: changes here might require changes in vfscanf.c!! */

    FLUSHONREADCHECK

    c = FGetC (fdesc->fcb->handle);
    if (c == EOF)
    {
        c = IoErr ();

        if (c)
        {
            errno = __stdc_ioerr2errno (c);

            stream->flags |= __POSIXC_STDIO_ERROR;
        }
        else
            stream->flags |= __POSIXC_STDIO_EOF;

        c = EOF;
    }

    return c;
} /* fgetc */

