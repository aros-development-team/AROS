/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function getline().
*/

/*****************************************************************************

    NAME */
#include <stdio.h>

        ssize_t getline (

/*  SYNOPSIS */
        char  ** restrict lineptr,
        size_t * restrict n,
        FILE  * restrict  stream)

/*  FUNCTION
        Read a line from stream, up to and including the next newline (or
        end-of-file), into a dynamically (re)allocated buffer. Equivalent to
        getdelim() with a delimiter of '\n'.

    INPUTS
        lineptr - address of a pointer to the buffer (allocated/grown as
                  needed; the caller must free() it).
        n       - address of the buffer's size.
        stream  - the stream to read from.

    RESULT
        The number of characters read, including the newline but not the
        terminating NUL, or -1 on error or end-of-file with nothing read.

    NOTES
        The result is always NUL-terminated.

    EXAMPLE

    BUGS

    SEE ALSO
        getdelim(), fgets()

    INTERNALS

******************************************************************************/
{
    return getdelim(lineptr, n, '\n', stream);
} /* getline */
