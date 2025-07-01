/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function wcslen().
*/

/*****************************************************************************

    NAME */
#include <wchar.h>

wchar_t *wcsncat(

/*  SYNOPSIS */
    wchar_t *dest,
    const wchar_t *src,
    size_t n)

/*  FUNCTION
        Appends at most n wide characters from the wide string src to the end
        of the wide string dest, and null-terminates the result.

    INPUTS
        dest - Destination wide string buffer. Must be large enough to hold the
               resulting concatenated string including the terminating null.
        src  - Source wide string to append.
        n    - Maximum number of wide characters to append from src.

    RESULT
        Returns a pointer to the destination string dest.

    NOTES
        Appends characters up to n or until a null wide character in src is
        encountered, whichever comes first. The resulting string in dest is
        always null-terminated. Behavior is undefined if dest and src overlap.

    EXAMPLE

        wchar_t buf[20] = L"Hello, ";
        wcsncat(buf, L"World!", 3);
        // buf now contains L"Hello, Wor"

    BUGS

    SEE ALSO
        wcscat(), wcsncpy()

    INTERNALS

******************************************************************************/
{
    wchar_t *d = dest;

    // Move to the end of dest
    while (*d) {
        d++;
    }

    // Append at most n chars from src
    while (n-- && *src) {
        *d++ = *src++;
    }

    // Null-terminate
    *d = L'\0';

    return dest;
}
