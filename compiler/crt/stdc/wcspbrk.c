/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function wcspbrk().
*/

/*****************************************************************************

    NAME */
#include <wchar.h>

wchar_t *wcspbrk(

/*  SYNOPSIS */
    const wchar_t *s,
    const wchar_t *accept)

/*  FUNCTION
        Scans the wide string s for the first occurrence of any character
        in the wide string accept.

    INPUTS
        s      - Wide string to scan.
        accept - Wide string containing characters to match.

    RESULT
        Returns a pointer to the first occurrence in s of any character from accept,
        or NULL if no such character is found.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        wcschr(), wcsrchr(), wmemchr()

    INTERNALS

******************************************************************************/
{
    while (*s) {
        const wchar_t *a = accept;
        while (*a) {
            if (*s == *a)
                return (wchar_t *)s;
            ++a;
        }
        ++s;
    }
    return NULL;
}
