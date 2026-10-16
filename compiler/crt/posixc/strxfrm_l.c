/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function strxfrm_l.
*/

#include <string.h>

/****************************************************************************
    NAME
        size_t strxfrm_l(

    SYNOPSIS
        char *dest, const char *src, size_t n, locale_t loc)

    FUNCTION
        Copy a byte string as a C-locale collation key.

    INPUTS
        dest - Destination buffer with room for n bytes; may be
               NULL when n is zero. It must not overlap src.
        src - Readable null-terminated byte string, including when
              n is zero.
        n - Destination capacity in bytes.
        loc - Ignored; no locale data is read.

    RESULT
        The length of src in bytes, excluding its null terminator.
        A return value less than n indicates a complete key.
        Otherwise, the destination must not be used as a complete
        null-terminated key.

    NOTES
        Include <string.h>. No storage is allocated. With n equal
        to zero, the function measures src without writing dest.
        Otherwise it copies up to n bytes and pads unused bytes
        with zeros. If the source length is at least n, no null
        terminator is written. Neither input pointer is retained.

    BUGS
        Locale-specific collation is not implemented. The function
        uses strxfrm() and ignores loc.

    SEE ALSO
        strxfrm(), strcoll_l(), wcsxfrm_l()

*****************************************************************************/
size_t strxfrm_l(char *dest, const char *src, size_t n, locale_t loc)
{
	return strxfrm(dest, src, n);
}
