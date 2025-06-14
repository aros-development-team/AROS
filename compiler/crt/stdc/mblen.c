/*
    Copyright (C) 2004-2025, The AROS Development Team. All rights reserved.

    Desc: AROS implementation of the C99 function mblen().
*/

#include <aros/debug.h>
#include <stddef.h>
#include <wchar.h>
#include <errno.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>

        int mblen(

/*  SYNOPSIS */
        const char *s,
        size_t n)

/*  FUNCTION
        Determines the number of bytes comprising the next multibyte character
        in the input string 's', examining at most 'n' bytes.

        This function is typically used to scan or measure multibyte strings
        when the actual wide character value is not needed.

    INPUTS
        s - Pointer to the multibyte character string to examine.
            If NULL, the function is used to test whether the current encoding
            has state-dependent encodings.

        n - Maximum number of bytes to examine in 's'.

    RESULT
        Returns the number of bytes that make up the next multibyte character
        if the sequence is valid.
        Returns 0 if the character is the null byte (`'\0'`).
        Returns -1 if the sequence is invalid or incomplete, and sets errno to EILSEQ.

        If 's' is NULL, returns 0 in UTF-8 or "C" locale, indicating a stateless encoding.

    NOTES
        stdc.library currently only implements "C" or UTF-8-compatible encodings.
        Therefore, this function always returns 0 when 's' is NULL and does not track shift states.
        The function relies on UTF-8 byte patterns to determine the multibyte length.

    EXAMPLE
        const char *mb = "ä";
        int len = mblen(mb, MB_CUR_MAX);
        if (len > 0) {
            // len indicates how many bytes 'ä' takes in the current encoding
        }

    BUGS
        Does not support locale-specific or stateful encodings.
        No shift state is maintained between calls, even in encodings where this might be necessary.

    SEE ALSO
        mbtowc(), mbstowcs(), wctomb(), wcstombs()

    INTERNALS
        If the first byte is ASCII (< 0x80), returns 1.
        For multibyte sequences, examines the leading byte to determine the expected length.
        Validates continuation bytes and rejects overlong or out-of-range encodings.

******************************************************************************/
{
    // Stateless implementation
    if (!s)
        return 0;

    if (n == 0)
        return -1;

    unsigned char c = (unsigned char)s[0];

    // ASCII fast path
    if (c < 0x80)
        return 1;

    int len;
    wchar_t wc = 0;

    if ((c & 0xE0) == 0xC0) {
        len = 2;
        wc = c & 0x1F;
    } else if ((c & 0xF0) == 0xE0) {
        len = 3;
        wc = c & 0x0F;
    } else if ((c & 0xF8) == 0xF0) {
        len = 4;
        wc = c & 0x07;
    } else {
        errno = EILSEQ;
        return -1; // Invalid start byte
    }

    if ((size_t)len > n)
        return -1; // Not enough bytes

    for (int i = 1; i < len; ++i) {
        if ((s[i] & 0xC0) != 0x80) {
            errno = EILSEQ;
            return -1; // Invalid continuation byte
        }
        wc = (wc << 6) | (s[i] & 0x3F);
    }

    // Reject overlong encodings and invalid ranges
    if ((len == 2 && wc < 0x80) ||
        (len == 3 && wc < 0x800) ||
        (len == 4 && wc < 0x10000) ||
        (wc > 0x10FFFF) ||
        (wc >= 0xD800 && wc <= 0xDFFF)) {
        errno = EILSEQ;
        return -1;
    }

    return len;
}

