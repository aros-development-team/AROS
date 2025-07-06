/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: AROS implementation of the C99 function mbrtowc().
*/

#include <stddef.h>
#include <wchar.h>
#include <errno.h>
#include <stdlib.h>

/*****************************************************************************

    NAME */
        size_t mbrtowc(

/*  SYNOPSIS */
        wchar_t * restrict pwc,
        const char * restrict s,
        size_t n,
        mbstate_t * restrict ps)

/*  FUNCTION
        Converts the multibyte character at 's' to a wide character stored in *pwc.

    INPUTS
        pwc - Pointer to output wide character (can be NULL).
        s   - Pointer to the input multibyte character.
        n   - Maximum number of bytes to examine from 's'.
        ps  - Conversion state object (unused in this stateless implementation).

    RESULT
        Returns the number of bytes consumed to produce the wide character.
        Returns 0 if the null character was converted and stored.
        Returns (size_t)-1 on encoding error and sets errno to EILSEQ.
        Returns (size_t)-2 if 's' is NULL or points to an incomplete character.

    NOTES
        Only UTF-8 multibyte sequences are supported.
        Locale-specific or stateful encodings are not implemented.

    EXAMPLE
        const char *mbs = "ä";
        wchar_t wc;
        size_t len = mbrtowc(&wc, mbs, strlen(mbs), NULL);

    SEE ALSO
        mbtowc(), mbstowcs(), wcrtomb()

******************************************************************************/
{
    /* We support only stateless UTF-8 decoding. */
    unsigned char c;
    wchar_t wc;
    size_t len;

    if (s == NULL)
        return 0; // No input, treat as null character

    if (n == 0)
        return (size_t)-2; // Incomplete multibyte character

    c = (unsigned char)s[0];

    /* ASCII fast path */
    if (c < 0x80) {
        if (pwc)
            *pwc = (wchar_t)c;
        return (c == '\0') ? 0 : 1;
    }

    /* UTF-8 decoding */
    if ((c & 0xE0) == 0xC0 && n >= 2) {
        if ((s[1] & 0xC0) != 0x80)
            goto ilseq;
        wc = ((c & 0x1F) << 6) | (s[1] & 0x3F);
        len = 2;
    } else if ((c & 0xF0) == 0xE0 && n >= 3) {
        if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80)
            goto ilseq;
        wc = ((c & 0x0F) << 12) |
             ((s[1] & 0x3F) << 6) |
             (s[2] & 0x3F);
        len = 3;
    } else if ((c & 0xF8) == 0xF0 && n >= 4) {
        if ((s[1] & 0xC0) != 0x80 ||
            (s[2] & 0xC0) != 0x80 ||
            (s[3] & 0xC0) != 0x80)
            goto ilseq;
        wc = ((c & 0x07) << 18) |
             ((s[1] & 0x3F) << 12) |
             ((s[2] & 0x3F) << 6) |
             (s[3] & 0x3F);
        len = 4;
    } else {
        /* Incomplete or invalid sequence */
        if (n < 2 || (c & 0xE0) == 0xC0) return (size_t)-2;
        if (n < 3 || (c & 0xF0) == 0xE0) return (size_t)-2;
        if (n < 4 || (c & 0xF8) == 0xF0) return (size_t)-2;
        goto ilseq;
    }

    if (pwc)
        *pwc = wc;

    return len;

ilseq:
    errno = EILSEQ;
    return (size_t)-1;
}
