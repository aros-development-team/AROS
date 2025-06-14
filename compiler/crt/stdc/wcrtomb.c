/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: AROS implementation of the C99 function wcrtomb().
*/
#include <stddef.h>
#include <wchar.h>
#include <errno.h>

/*****************************************************************************

    NAME */
size_t wcrtomb(

/*  SYNOPSIS */
    char * restrict s, 
    wchar_t wc, 
    mbstate_t * restrict ps)

/*  FUNCTION
        Converts a single wide character (wchar_t) to its multibyte representation
        and stores it in the output buffer 's'.

    INPUTS
        s  - Pointer to the destination buffer to receive the multibyte sequence.
             If NULL, the function returns the length of the encoding sequence used
             to reset the shift state (which is 1 for UTF-8 or 0 for state-independent encodings).
        wc - The wide character to convert.
        ps - Pointer to an mbstate_t object that keeps conversion state between calls.
             May be NULL for stateless encodings like UTF-8.

    RESULT
        Returns the number of bytes written to 's' if successful.
        If 'wc' is the null wide character, a single null byte is written and 1 is returned.
        If the conversion fails (invalid code point or sequence), (size_t)-1 is returned
        and errno is set to EILSEQ.

    NOTES
        This implementation assumes a stateless encoding (UTF-8).
        If 'ps' is NULL, an internal static conversion state is not maintained — behavior is stateless.
        Surrogate pairs and values above U+10FFFF are treated as invalid.

    EXAMPLE
        wchar_t wc = L'ü';
        char buf[MB_CUR_MAX];
        size_t len = wcrtomb(buf, wc, NULL);

    BUGS
        Does not support stateful encodings.
        No locale-specific behavior is implemented — assumes UTF-8 or ASCII compatible only.

    SEE ALSO
        wctomb(), mbtowc(), mbrtowc(), wcstombs(), mbstowcs()

    INTERNALS
        Builds the UTF-8 sequence from the high-order bits of the wchar_t value.
        Validates character ranges against Unicode limits.

******************************************************************************/
{
    unsigned int bits, j, k;
    unsigned char c;

    // Stateless implementation
    (void)ps;

    // Check for invalid Unicode range
    if (wc > 0x10FFFF || (wc >= 0xD800 && wc <= 0xDFFF)) {
        errno = EILSEQ;
        return (size_t)-1;
    }

    // Handle ASCII fast path
    if (wc < 0x80) {
        if (s)
            *s = (char)wc;
        return 1;
    }

    // Determine number of bytes and initial prefix
    if (wc < 0x800) {
        bits = 6;
        c = 0xC0;
        j = 2;
    } else if (wc < 0x10000) {
        bits = 12;
        c = 0xE0;
        j = 3;
    } else {
        bits = 18;
        c = 0xF0;
        j = 4;
    }

    // If s == NULL, return number of bytes needed
    if (!s)
        return j;

    // Write leading byte
    s[0] = c | (wc >> bits);

    // Write continuation bytes
    for (k = 1; k < j; ++k) {
        bits -= 6;
        s[k] = 0x80 | ((wc >> bits) & 0x3F);
    }

    return j;
}
