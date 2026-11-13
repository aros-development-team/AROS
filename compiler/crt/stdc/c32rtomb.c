/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    C11 function c32rtomb().
*/

#include <stddef.h>
#include <wchar.h>

/*****************************************************************************

    NAME */
#include <uchar.h>

	size_t c32rtomb (

/*  SYNOPSIS */
	char * restrict s,
	char32_t c32,
	mbstate_t * restrict ps)

/*  FUNCTION
	Converts a single 32-bit (UTF-32) character to its multibyte
	representation.

    INPUTS
	s    - Where to store the multibyte sequence (may be NULL).
	c32  - The character to convert.
	ps   - Conversion state (a static internal state is used if NULL).

    RESULT
	The number of bytes stored, or (size_t)-1 on an encoding error
	(errno = EILSEQ).

    NOTES
	char32_t holds a complete Unicode code point, so on AROS - where
	wchar_t is 32 bits wide - this is equivalent to wcrtomb().

    EXAMPLE

    BUGS

    SEE ALSO
	c16rtomb(), mbrtoc32(), wcrtomb()

    INTERNALS

******************************************************************************/
{
    return wcrtomb(s, (wchar_t)c32, ps);
}
