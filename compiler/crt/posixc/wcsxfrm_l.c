/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function wcsxfrm_l.
*/

#include <wchar.h>

/****************************************************************************
    NAME
        size_t wcsxfrm_l(

    SYNOPSIS
        wchar_t *dest, const wchar_t *src, size_t n, locale_t loc)

    FUNCTION
        Copy a wide-character string as a C-locale collation key.

    INPUTS
        dest - Destination buffer with room for n wchar_t elements.
               Use NULL with n equal to zero to measure the key.
               Source and destination must not overlap.
        src - Readable null-terminated wide-character string,
              including when n is zero.
        n - Destination capacity in wchar_t elements, not bytes.
        loc - Ignored; no locale data is read.

    RESULT
        The length of src in wchar_t elements, excluding its null
        terminator. A return value less than n indicates a complete
        key if dest was supplied. A terminated prefix is not a
        complete key when the return value is at least n.

    NOTES
        Include <wchar.h> with _POSIX_C_SOURCE set to at least
        200809L. No storage is allocated and no pointer is retained.

        When dest is non-NULL and n is positive, the function copies
        at most n - 1 elements and appends a null wide character.
        The remaining buffer elements are unchanged. With n equal
        to zero or dest equal to NULL, it only measures src.

    BUGS
        Locale-specific collation is not implemented. The function
        uses wcsxfrm() and ignores loc.

    SEE ALSO
        wcsxfrm(), wcscoll_l(), strxfrm_l()

*****************************************************************************/
size_t wcsxfrm_l(wchar_t *dest, const wchar_t *src, size_t n, locale_t loc)
{
    return wcsxfrm(dest, src, n);
}
