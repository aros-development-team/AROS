/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function iswlower_l.
*/

#include <wctype.h>

/****************************************************************************
    NAME
        int iswlower_l(

    SYNOPSIS
        wint_t wc, locale_t locale)

    FUNCTION
        Test whether the current C locale classifies wc as
        a lowercase letter.

    INPUTS
        wc - Wide character value to classify, or WEOF.
        locale - Locale handle. Ignored by this implementation;
                 it is neither dereferenced nor retained.

    RESULT
        1 if wc belongs to the requested class; otherwise 0.
        WEOF and values outside the classification table return
        0. No error is reported through errno.

    NOTES
        Include <wctype.h> with _POSIX_C_SOURCE set to at least
        200809L. Classification is limited to U+0000 through
        U+00FF when __WCHAR_MAX__ exceeds 255, and to U+0000
        through U+007F otherwise. Selecting C.UTF-8 does not
        extend this range.

    BUGS
        The supplied locale does not select the classification
        table. The result follows the current C locale instead.
        The built-in fallback table omits U+00B5 and U+00FF.

    SEE ALSO
        iswlower(), iswctype(), newlocale()

*****************************************************************************/
int iswlower_l(wint_t wc, locale_t locale)
{
	return iswlower(wc);
}
