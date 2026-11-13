/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    C11 function c16rtomb().
*/

#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <wchar.h>

/*****************************************************************************

    NAME */
#include <uchar.h>

	size_t c16rtomb (

/*  SYNOPSIS */
	char * restrict s,
	char16_t c16,
	mbstate_t * restrict ps)

/*  FUNCTION
	Converts a 16-bit (UTF-16) code unit to its multibyte representation.
	A high surrogate is buffered until the following low surrogate is
	supplied, at which point the combined code point is emitted.

    INPUTS
	s    - Where to store the multibyte sequence (may be NULL).
	c16  - The UTF-16 code unit to convert.
	ps   - Conversion state (a static internal state is used if NULL).

    RESULT
	The number of bytes stored (0 when a high surrogate is buffered and no
	output is produced yet), or (size_t)-1 on an encoding error
	(errno = EILSEQ).

    NOTES
	Only UTF-8 output is supported.

    EXAMPLE

    BUGS

    SEE ALSO
	c32rtomb(), mbrtoc16(), wcrtomb()

    INTERNALS

******************************************************************************/
{
    static mbstate_t internal;

    if (ps == NULL)
        ps = &internal;

    if (ps->__state != 0)
    {
        /* A high surrogate is pending; this unit must be the low surrogate. */
        if (c16 >= 0xDC00 && c16 <= 0xDFFF)
        {
            uint_least32_t hi = (uint_least32_t)ps->__value;
            uint_least32_t cp = 0x10000u
                + (((hi - 0xD800u) << 10) | ((uint_least32_t)c16 - 0xDC00u));
            ps->__state = 0;
            ps->__value = 0;
            return wcrtomb(s, (wchar_t)cp, NULL);
        }

        /* Not a low surrogate: malformed UTF-16. */
        ps->__state = 0;
        ps->__value = 0;
        errno = EILSEQ;
        return (size_t)-1;
    }

    if (c16 >= 0xD800 && c16 <= 0xDBFF)
    {
        /* High surrogate: remember it and wait for the low surrogate. */
        ps->__state = 1;
        ps->__value = (wchar_t)c16;
        return 0;
    }

    if (c16 >= 0xDC00 && c16 <= 0xDFFF)
    {
        /* Unpaired low surrogate. */
        errno = EILSEQ;
        return (size_t)-1;
    }

    return wcrtomb(s, (wchar_t)c16, NULL);
}
