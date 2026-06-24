/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

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
        Implemented in terms of mbrtowc(NULL, s, n, ps) as required by
        C99 7.24.6.2, so the two functions always agree on what constitutes a
        valid sequence (including overlong/surrogate/out-of-range rejection).

    EXAMPLE
        const char *mbs = "ä";
        size_t len = mbrlen(mbs, strlen(mbs), NULL);

    SEE ALSO
        mbrtowc(), mbtowc(), mbstowcs()

******************************************************************************/
{
    static mbstate_t __mbrlen_state;

    return mbrtowc(NULL, s, n, ps ? ps : &__mbrlen_state);
}
