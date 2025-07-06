/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: AROS implementation of the C99 function mbrlen().
*/

#include <stddef.h>
#include <wchar.h>
#include <errno.h>

/*****************************************************************************

    NAME */
        size_t mbrlen(

/*  SYNOPSIS */
        const char * restrict s,
        size_t n,
        mbstate_t * restrict ps)

/*  FUNCTION
        Determines the number of bytes needed to represent the next multibyte
        character in the input string 's'.

    INPUTS
        s   - Pointer to the input multibyte string.
        n   - Maximum number of bytes to examine from 's'.
        ps  - Conversion state object (unused in this stateless implementation).

    RESULT
        Returns the number of bytes that comprise the next multibyte character.
        Returns 0 if the character is the null character.
        Returns (size_t)-1 on encoding error and sets errno to EILSEQ.
        Returns (size_t)-2 if more bytes are needed to complete a valid character.

    NOTES
        Only UTF-8 multibyte sequences are supported.
        The encoding is stateless; mbstate_t is unused.

    EXAMPLE
        const char *mbs = "ä";
        size_t len = mbrlen(mbs, strlen(mbs), NULL);

    SEE ALSO
        mbrtowc(), mbtowc(), mbstowcs()

******************************************************************************/
{
    unsigned char c;

    if (s == NULL || *s == '\0')
        return 0;

    if (n == 0)
        return (size_t)-2;

    c = (unsigned char)s[0];

    /* ASCII character (1-byte UTF-8) */
    if (c < 0x80)
        return 1;

    /* Check UTF-8 multibyte sequence length */
    if ((c & 0xE0) == 0xC0) {
        if (n < 2) return (size_t)-2;
        if ((s[1] & 0xC0) != 0x80) goto ilseq;
        return 2;
    }

    if ((c & 0xF0) == 0xE0) {
        if (n < 3) return (size_t)-2;
        if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80) goto ilseq;
        return 3;
    }

    if ((c & 0xF8) == 0xF0) {
        if (n < 4) return (size_t)-2;
        if ((s[1] & 0xC0) != 0x80 ||
            (s[2] & 0xC0) != 0x80 ||
            (s[3] & 0xC0) != 0x80) goto ilseq;
        return 4;
    }

ilseq:
    errno = EILSEQ;
    return (size_t)-1;
}
