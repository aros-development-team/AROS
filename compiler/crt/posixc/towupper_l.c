/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function towupper_l().
*/

#include <aros/types/wchar_t.h>
#include <aros/types/wint_t.h>

#define STDC_NOINLINE_WCTYPE

/*****************************************************************************

    NAME */
#include <wctype.h>

        wint_t towupper_l(

/*  SYNOPSIS */
        wint_t wc,
        locale_t locale)

/*  FUNCTION
        Converts a lowercase wide character to its uppercase equivalent.

    INPUTS
        wc - a wide character code point.

    RESULT
        The uppercase equivalent of the input character if applicable;
        otherwise returns the character unchanged.

    NOTES
        Supports all characters in the ASCII and Latin-1 Supplement ranges.
        For example, `L'ö'` ? `L'Ö'`.

    EXAMPLE
        wint_t c = towupper(L'è');  // returns L'È'

    BUGS
        Characters outside U+00FF are returned unchanged.

    SEE ALSO
        towlower(), towctrans()

    INTERNALS
        Uses mapping tables specified in locales stored in stdc's library base.
        Defaults to a 128/256-entry static mapping table(s) covering
        U+0000 to U+00FF, (basic Latin) and 
        Latin-1 mappings.

******************************************************************************/
{
    return towupper(wc);
}
