/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    C11 function mbrtoc16().
*/

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

/*****************************************************************************

    NAME */
#include <uchar.h>

	size_t mbrtoc16 (

/*  SYNOPSIS */
	char16_t * restrict pc16,
	const char * restrict s,
	size_t n,
	mbstate_t * restrict ps)

/*  FUNCTION
	Converts a single multibyte character to a 16-bit (UTF-16) character.
	Code points outside the Basic Multilingual Plane are returned as a
	UTF-16 surrogate pair over two successive calls.

    INPUTS
	pc16 - Where to store the converted code unit (may be NULL).
	s    - Multibyte sequence to convert, or NULL.
	n    - Maximum number of bytes from 's' to examine.
	ps   - Conversion state (a static internal state is used if NULL).

    RESULT
	The number of bytes consumed, 0 for a null character, (size_t)-1 on an
	encoding error (errno = EILSEQ), (size_t)-2 for an incomplete sequence,
	or (size_t)-3 when the stored code unit is the low surrogate produced
	from a preceding call (no further input is consumed).

    NOTES
	Only UTF-8 multibyte sequences are supported.

    EXAMPLE

    BUGS

    SEE ALSO
	mbrtoc32(), c16rtomb(), mbrtowc()

    INTERNALS

******************************************************************************/
{
    static mbstate_t internal;
    wchar_t wc = 0;
    size_t r;

    if (ps == NULL)
        ps = &internal;

    /* A preceding call produced a high surrogate and stashed the matching
       low surrogate; return it now without consuming any input. */
    if (ps->__state != 0)
    {
        if (pc16 != NULL)
            *pc16 = (char16_t)ps->__value;
        ps->__state = 0;
        ps->__value = 0;
        return (size_t)-3;
    }

    r = mbrtowc(&wc, s, n, ps);
    if (r == (size_t)-1 || r == (size_t)-2)
        return r;

    if ((uint_least32_t)wc > 0xFFFF)
    {
        /* Encode as a UTF-16 surrogate pair: emit the high surrogate now
           and remember the low surrogate for the next call. */
        uint_least32_t v = (uint_least32_t)wc - 0x10000;
        if (pc16 != NULL)
            *pc16 = (char16_t)(0xD800 + (v >> 10));
        ps->__state = 1;
        ps->__value = (wchar_t)(0xDC00 + (v & 0x3FF));
        return r;
    }

    if (pc16 != NULL)
        *pc16 = (char16_t)wc;
    return r;
}
