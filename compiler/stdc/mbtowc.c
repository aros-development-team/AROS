/*
    Copyright © 2007-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function mbtowc().
*/

/*****************************************************************************

    NAME
#include <stdlib.h>

	int mbtowc(

    SYNOPSIS
	wchar_t * restrict pwc,
	const char * restrict s,
	size_t n)

    FUNCTION
        C99 mbtowc function; a function to convert one multi-byte character
        to a wchar_t character and/or to determine the number of bytes for the
        next multi-byte char.

    INPUTS
        pwc: pointer wide char string to put converted char in. When NULL
             no char will be converted.
        s: pointer to multi-byte char as input
        n: maximum of bytes to look at for the multi-byte char.
        
    RESULT
        If s is not NULL the function returns the number of bytes the next
        multi-byte character is made of; 0 if the char pointed to is NULL or
        -1 if it is not a valid multi-byte char.
        If s is NULL the function return zero or non-zero when multi-byte chars
        resp. don't or do have state-dependent encodings.

    NOTES
	stdc.library currenlty only supports "C" locale
        This means that the function returns 0 when s is NULL and only 0, 1 or -1
        when s is not NULL.

    EXAMPLE

    BUGS

    SEE ALSO
        wctomb()

    INTERNALS
        Implemented as static inline function to adapt to changing wchar_t
        definitions

******************************************************************************/

