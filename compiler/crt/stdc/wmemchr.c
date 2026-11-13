/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

/*****************************************************************************

    NAME */
#include <wchar.h>

wchar_t *wmemchr(

/*  SYNOPSIS */
    const wchar_t *s,
    wchar_t c,
    size_t n)

/*  FUNCTION
        Scans the initial n wide characters of the memory area pointed to by s
        for the first occurrence of the wide character c.

    INPUTS
        s - Memory area to scan.
        c - Wide character to locate.
        n - Number of wide characters to scan.

    RESULT
        Returns a pointer to the first occurrence of c within the first n
        wide characters of s, or NULL if c is not found.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        wmemcpy(), wmemmove()

    INTERNALS

******************************************************************************/
{
    while (n-- > 0) {
        if (*s == c)
            return (wchar_t *)s;
        ++s;
    }
    return NULL;
}
