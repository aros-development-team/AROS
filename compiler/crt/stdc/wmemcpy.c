/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function wmemcpy().
*/

#include <string.h>

/*****************************************************************************

    NAME */
#include <wchar.h>

wchar_t *wmemcpy(

/*  SYNOPSIS */
    wchar_t * restrict s1,
    const wchar_t * restrict s2,
    size_t n)

/*  FUNCTION
        Copies n wide characters from s2 to s1.
        The memory areas must not overlap.

    INPUTS
        s1 - Destination buffer.
        s2 - Source buffer.
        n  - Number of wide characters to copy.

    RESULT
        Returns s1.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        wmemmove()

    INTERNALS

******************************************************************************/
{
    return (wchar_t *)memcpy(s1, s2, n * sizeof(wchar_t));
}
