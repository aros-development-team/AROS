/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 wcsxfrm.
*/

/*****************************************************************************

    NAME */
#include <wchar.h>

        size_t wcsxfrm(

/*  SYNOPSIS */
        wchar_t *dest, const wchar_t *src, size_t n)

/*  FUNCTION
        Transforms the wide-character string 'src' into a form suitable
        for comparison using wcscmp(), and stores it in 'dest'.

    INPUTS
        dest -- destination buffer to store the transformed string
        src  -- source wide-character string
        n    -- maximum number of characters to store in dest

    RESULT
        Returns the length of the transformed string (as if no limit was applied),
        not counting the null terminator.

    NOTES
        This naive implementation simply copies 'src' to 'dest' up to
        'n' characters (including the null terminator if space permits).
        This implementation does not perform locale-specific transformations;
        it is equivalent to wcsncpy() + wcslen()

    EXAMPLE

    BUGS

    SEE ALSO
        wcscmp(), wcscoll(), wcscpy(), wcsncpy()

    INTERNALS

******************************************************************************/
{
    size_t len = 0;

    while (src[len] != L'\0')
        len++;

    if (dest != NULL && n > 0) {
        size_t i;
        for (i = 0; i < len && i < n - 1; i++)
            dest[i] = src[i];
        if (i < n)
            dest[i] = L'\0';
    }

    return len;
}
