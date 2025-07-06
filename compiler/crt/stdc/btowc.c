/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: AROS implementation of the C99 function btowc().
*/

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

/*****************************************************************************

    NAME */
#include <wchar.h>

wint_t btowc(

/*  SYNOPSIS */
        int c)

/*    FUNCTION
        The btowc() function converts a single byte character (in the range
        of unsigned char) to its corresponding wide character representation,
        according to the current multibyte encoding in the current locale.

        If the input is EOF or does not correspond to a valid single-byte
        multibyte character, WEOF is returned.

    INPUTS
        c - A single byte character as an int (must be representable as
            unsigned char or EOF).

    RESULT
        Returns the corresponding wide character (wint_t) if the conversion
        succeeds. Returns WEOF if the input is EOF or an invalid multibyte
        sequence.

    NOTES
        This function only works for single-byte multibyte characters. It is
        intended to be used with byte input where the encoding may vary
        depending on the locale.

        It uses mbrtowc() internally with a zero-initialized conversion state.

    EXAMPLE
        wint_t wc = btowc((int)'A');

    BUGS
        If called with a multibyte encoding that requires more than one byte
        per character (e.g., UTF-8 for non-ASCII), this function will fail
        for non-ASCII characters.

    SEE ALSO
        wctob(), mbrtowc(), setlocale()

    INTERNALS
        Uses mbrtowc() with a state object initialized to zero. Only attempts
        to convert 1 byte. Returns WEOF on encoding error or if input is EOF.

******************************************************************************/
{
    unsigned char byte = (unsigned char)c;
    wchar_t wc;
    mbstate_t state = {0};

    if (c == EOF)
        return WEOF;

    if (mbrtowc(&wc, (const char *)&byte, 1, &state) == (size_t)-1)
        return WEOF;

    return wc;
}
