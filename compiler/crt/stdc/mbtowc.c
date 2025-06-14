/*
    Copyright (C) 2007-2025, The AROS Development Team. All rights reserved.

    Desc: AROS implementation of the C99 function mbtowc().
*/
#include <stddef.h>
#include <wchar.h>
#include <errno.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>

        int mbtowc(

/*  SYNOPSIS */
        wchar_t * restrict pwc,
        const char * restrict s,
        size_t n)

/*  FUNCTION
        Converts the multibyte sequence starting at 's' into a wide character and stores
        it at the location pointed to by 'pwc'.

        The function examines up to 'n' bytes to determine the length of the multibyte
        character. If 's' is NULL, the function is used to reset the conversion state.

    INPUTS
        pwc - Pointer to the location where the resulting wide character will be stored.
              If NULL, the function still parses the multibyte sequence to determine length,
              but does not store the result.

        s   - Pointer to the multibyte character sequence to convert.
              If NULL, the function returns 0 (for stateless encodings like UTF-8).

        n   - Maximum number of bytes to examine from 's'.

    RESULT
        Returns the number of bytes that were consumed to produce a valid wide character.
        Returns 0 if the character is the null byte (`'\0'`).
        Returns -1 if an invalid or incomplete multibyte sequence is encountered, and sets errno to EILSEQ.

    NOTES
        stdc.library currently only implements "C" or UTF-8-compatible locales.
        The encoding is treated as stateless, so no shift state is used or stored.

        This function is not thread-safe with stateful encodings unless all calls use the same static state,
        which is not applicable here.

    EXAMPLE
        const char *mb = "??";
        wchar_t wc;
        int len = mbtowc(&wc, mb, MB_CUR_MAX);
        if (len > 0) {
            // wc now holds the wide character value for '??'
        }

    BUGS
        Does not support stateful or locale-dependent encodings.
        Returns -1 on partial or invalid sequences without giving detailed error context.

    SEE ALSO
        mblen(), mbstowcs(), wctomb(), wcstombs()

    INTERNALS
        Parses the UTF-8 byte sequence using its leading byte to determine the expected length.
        Validates each continuation byte and reconstructs the Unicode code point into 'pwc'.
        Performs range checks to avoid accepting overlong or invalid sequences.

******************************************************************************/
{
    static mbstate_t ps;

    // If s == NULL, reset state — not used in our case
    if (!s)
        return 0;

    if (n == 0)
        return -1;

    unsigned char c = (unsigned char)s[0];

    // ASCII fast path
    if (c < 0x80) {
        if (pwc) *pwc = c;
        return 1;
    }

    int len;
    wchar_t wc = 0;

    if ((c & 0xE0) == 0xC0) {         // 2-byte
        len = 2;
        wc = c & 0x1F;
    } else if ((c & 0xF0) == 0xE0) {  // 3-byte
        len = 3;
        wc = c & 0x0F;
    } else if ((c & 0xF8) == 0xF0) {  // 4-byte
        len = 4;
        wc = c & 0x07;
    } else {
        errno = EILSEQ;
        return -1;
    }

    if ((size_t)len > n) {
        errno = EILSEQ;
        return -1; // Not enough bytes
    }

    for (int i = 1; i < len; ++i) {
        if ((s[i] & 0xC0) != 0x80) {
            errno = EILSEQ;
            return -1; // Invalid continuation byte
        }
        wc = (wc << 6) | (s[i] & 0x3F);
    }

    // Reject overlong encodings and invalid codepoints
    if ((len == 2 && wc < 0x80) ||
        (len == 3 && wc < 0x800) ||
        (len == 4 && wc < 0x10000) ||
        (wc > 0x10FFFF) || (wc >= 0xD800 && wc <= 0xDFFF)) {
        errno = EILSEQ;
        return -1;
    }

    if (pwc)
        *pwc = wc;

    return len;
}
