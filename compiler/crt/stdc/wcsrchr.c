/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function wcsrchr().
*/

/*****************************************************************************

    NAME */
#include <wchar.h>

wchar_t *wcsrchr(

/*  SYNOPSIS */
    const wchar_t *s,
    wchar_t c)

/*  FUNCTION
        Locates the last occurrence of the wide character c in the wide string s.

    INPUTS
        s - Wide string to search.
        c - Wide character to locate.

    RESULT
        Returns a pointer to the last occurrence of c in s,
        or NULL if c is not found. If c is the null character,
        returns a pointer to the null terminator.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        wcschr(), wmemchr()

    INTERNALS

******************************************************************************/
{
    const wchar_t *last = NULL;
    while (*s) {
        if (*s == c)
            last = s;
        ++s;
    }
    if (c == L'\0')
        return (wchar_t *)s;
    return (wchar_t *)last;
}
