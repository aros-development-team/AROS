/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function dprintf().
*/

/*****************************************************************************

    NAME */
#include <stdio.h>
#include <stdarg.h>

        int dprintf (

/*  SYNOPSIS */
        int                fd,
        const char * restrict format,
        ...)

/*  FUNCTION
        Format output according to format and write it to the file
        descriptor fd. Equivalent to fprintf() but operating on a POSIX file
        descriptor rather than a FILE stream.

    INPUTS
        fd     - the file descriptor to write to.
        format - printf()-style format string.
        ...    - the arguments consumed by the format string.

    RESULT
        The number of bytes written, or a negative value on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        vdprintf(), fprintf(), write()

    INTERNALS

******************************************************************************/
{
    va_list args;
    int     ret;

    va_start(args, format);
    ret = vdprintf(fd, format, args);
    va_end(args);

    return ret;
} /* dprintf */
