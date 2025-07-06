/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function towlower().
*/

#include <aros/debug.h>
#include <clib/macros.h>

#include <aros/types/wchar_t.h>
#include <aros/types/wint_t.h>

#define STDC_NOINLINE_WCTYPE

#include "__stdc_intbase.h"

/*****************************************************************************

    NAME */
#include <wctype.h>

        wint_t towlower(

/*  SYNOPSIS */
        wint_t wc)

/*  FUNCTION
        Converts an uppercase wide character to its lowercase equivalent.

    INPUTS
        wc - a wide character code point.

    RESULT
        The lowercase equivalent of the input character if applicable;
        otherwise returns the character unchanged.

    NOTES
        Supports all characters in the ASCII and Latin-1 Supplement ranges.
        For example, `L'É'` ? `L'é`.

    EXAMPLE
        wint_t c = towlower(L'Ä');  // returns L'ä'

    BUGS

    SEE ALSO
        towupper(), towctrans()

    INTERNALS
        Uses a mapping table specified in stdc's library base.
        Defaults to a 256-entry static mapping table covering
        U+0000 to U+00FF, allowing only basic Latin and 
        Latin-1 mappings.

******************************************************************************/
{
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();

    if (StdCBase->__locale_cur && StdCBase->__locale_cur->__lc_tbl_u2l && (wc >= 0 && wc <= StdCBase->__locale_cur->__lc_tbl_size)) {
        wint_t mapped = StdCBase->__locale_cur->__lc_tbl_u2l[wc];
        return mapped ? mapped : wc;
    }
    return wc;
}
