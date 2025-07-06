/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    C99 function vfprintf()
*/
/* Original source from libnix */


#include <proto/dos.h>
#include <errno.h>
#include <stdarg.h>
#include "__fdesc.h"
#include "__stdio.h"

static int __putc(int c, void *fh);

/*****************************************************************************

    NAME */
#include <stdio.h>

        int vfprintf (

/*  SYNOPSIS */
        FILE       * stream,
        const char * format,
        va_list      args)

/*  FUNCTION
        Writes formatted output to the specified output stream using the format
        string and a `va_list` of arguments.

    INPUTS
        stream - A pointer to an open output stream.
        format - A format string, as used in printf().
        args   - A `va_list` of arguments to be formatted and printed.

    RESULT
        Returns the number of characters written, or 0 if an error occurred
        (e.g., invalid stream).

    NOTES
        - The stream must be writable and valid.
        - This function is typically called by `fprintf` and `printf` internally.
        - Format specifiers follow standard printf-style conventions.

    EXAMPLE
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stdout, fmt, ap);
        va_end(ap);

    BUGS
        - Returns 0 instead of the standard -1 on error, which may cause
          incorrect error checking by compliant code.
        - Not thread-safe if stream is shared among threads without synchronization.
        - Assumes AROS-specific file descriptor abstraction and output handling.

    SEE ALSO
        fprintf(), printf(), vprintf(),
        stdc.library/vsprintf(), stdc.library/vsnprintf(),
        va_start(), va_list

    INTERNALS
        - Retrieves the file descriptor wrapper via `__getfdesc()`.
        - Uses an AROS-specific `__vcformat()` function with `__putc()` as a
          character output callback.
        - `__putc()` uses `FPutC()` on the AROS file handle, converting AROS I/O
          errors via `__stdc_ioerr2errno()`.

******************************************************************************/
{
    fdesc *fdesc = __getfdesc(stream->fd);

    if (!fdesc)
    {
        errno = EBADF;
        return 0;
    }

    return __vcformat ((void *)BADDR(fdesc->fcb->handle), __putc, format, args);
} /* vfprintf */


static int __putc(int c, void *fhp)
{
    BPTR fh = MKBADDR(fhp);
    if (FPutC(fh, c) == EOF)
    {
        errno = __stdc_ioerr2errno(IoErr());
        return EOF;
    }

    return c;
}
