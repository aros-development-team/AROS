/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function vdprintf().
*/

#include <stdlib.h>
#include <unistd.h>

/*****************************************************************************

    NAME */
#include <stdio.h>
#include <stdarg.h>

        int vdprintf (

/*  SYNOPSIS */
        int                fd,
        const char * restrict format,
        va_list            args)

/*  FUNCTION
        Format output according to format and write it to the file
        descriptor fd. Equivalent to vfprintf() but operating on a POSIX file
        descriptor rather than a FILE stream.

    INPUTS
        fd     - the file descriptor to write to.
        format - printf()-style format string.
        args   - the variable argument list.

    RESULT
        The number of bytes written, or a negative value on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        dprintf(), vfprintf(), write()

    INTERNALS
        Formats into a heap buffer with vasprintf() and write()s it.

******************************************************************************/
{
    char   *buf = NULL;
    int     len;
    ssize_t written;

    len = vasprintf(&buf, format, args);
    if (len < 0)
        return -1;

    written = write(fd, buf, (size_t)len);
    free(buf);

    if (written != (ssize_t)len)
        return -1;

    return len;
} /* vdprintf */
