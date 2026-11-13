/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    C11 function mbrtoc32().
*/

#include <stddef.h>
#include <wchar.h>

/*****************************************************************************

    NAME */
#include <uchar.h>

	size_t mbrtoc32 (

/*  SYNOPSIS */
	char32_t * restrict pc32,
	const char * restrict s,
	size_t n,
	mbstate_t * restrict ps)

/*  FUNCTION
	Converts a single multibyte character to a 32-bit (UTF-32) character.

    INPUTS
	pc32 - Where to store the converted character (may be NULL).
	s    - Multibyte sequence to convert, or NULL.
	n    - Maximum number of bytes from 's' to examine.
	ps   - Conversion state (a static internal state is used if NULL).

    RESULT
	As mbrtowc(): the number of bytes consumed, 0 for a null character,
	(size_t)-1 on an encoding error (errno = EILSEQ) or (size_t)-2 for an
	incomplete sequence.

    NOTES
	char32_t holds a complete Unicode code point, so on AROS - where
	wchar_t is 32 bits wide - this is equivalent to mbrtowc().

    EXAMPLE

    BUGS

    SEE ALSO
	mbrtoc16(), c32rtomb(), mbrtowc()

    INTERNALS

******************************************************************************/
{
    wchar_t wc = 0;
    size_t r = mbrtowc(&wc, s, n, ps);

    if (r != (size_t)-1 && r != (size_t)-2 && pc32 != NULL)
        *pc32 = (char32_t)wc;

    return r;
}
