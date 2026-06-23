/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function getdelim().
*/

#include <errno.h>
#include <stdlib.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

        ssize_t getdelim (

/*  SYNOPSIS */
        char  ** restrict lineptr,
        size_t * restrict n,
        int               delim,
        FILE  * restrict  stream)

/*  FUNCTION
        Read characters from stream up to and including the first occurrence
        of the delimiter byte delim (or end-of-file), storing them in a
        dynamically (re)allocated buffer.

    INPUTS
        lineptr - address of a pointer to the buffer. If *lineptr is NULL (or
                  *n is 0) a buffer is allocated; otherwise it is grown with
                  realloc() as needed. The caller must free() it.
        n       - address of the buffer's size; updated when it is (re)grown.
        delim   - the delimiter byte at which reading stops.
        stream  - the stream to read from.

    RESULT
        The number of characters read, including the delimiter but not the
        terminating NUL, or -1 on error or end-of-file with nothing read
        (errno is set on error).

    NOTES
        The result is always NUL-terminated.

    EXAMPLE

    BUGS

    SEE ALSO
        getline(), fgetc(), fgets()

    INTERNALS

******************************************************************************/
{
    char   *buf;
    size_t  cap, len = 0;
    int     c;

    if (lineptr == NULL || n == NULL || stream == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    buf = *lineptr;
    cap = *n;

    if (buf == NULL || cap == 0)
    {
        cap = 120;
        buf = malloc(cap);
        if (buf == NULL)
        {
            errno = ENOMEM;
            return -1;
        }
        *lineptr = buf;
        *n = cap;
    }

    while ((c = fgetc(stream)) != EOF)
    {
        /* Ensure room for the new character plus the terminating NUL. */
        if (len + 1 >= cap)
        {
            size_t newcap = cap * 2;
            char  *newbuf = realloc(buf, newcap);

            if (newbuf == NULL)
            {
                errno = ENOMEM;
                return -1;
            }
            buf = newbuf;
            cap = newcap;
            *lineptr = buf;
            *n = cap;
        }

        buf[len++] = (char)c;
        if (c == delim)
            break;
    }

    if (len == 0)
        return -1;      /* end-of-file or error, nothing read */

    buf[len] = '\0';
    return (ssize_t)len;
} /* getdelim */
