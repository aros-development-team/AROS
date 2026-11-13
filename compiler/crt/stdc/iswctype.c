/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function iswctype().
*/

#include <aros/debug.h>
#include <clib/macros.h>

#include <aros/types/wchar_t.h>
#include <aros/types/wctype_t.h>
#include <aros/types/wint_t.h>

#define STDC_NOINLINE_WCTYPE

#include "__stdc_intbase.h"

/*****************************************************************************

    NAME */
#include <wctype.h>

        int iswctype(

/*  SYNOPSIS */
        wint_t wc, wctype_t desc)

/*  FUNCTION
        Tests whether a wide character `wc` has the property indicated by
        `desc`, which must be a value returned by `wctype()`.

    INPUTS
        wc    - the wide character code point to test.
        desc  - classification descriptor returned by `wctype()`.

    RESULT
        Non-zero if the character has the property; zero otherwise.

    NOTES
        The `desc` value must be obtained from a prior call to `wctype()`.
        Behavior is undefined if `desc` is invalid or comes from a different
        implementation.

    EXAMPLE
        if (iswctype(L'9', wctype("digit"))) {
            // Character is a digit
        }

    BUGS
        Only standard ASCII and Latin-1 Supplement classifications are
        supported. No locale-specific behavior or Unicode categories beyond
        U+00FF are recognized.

    SEE ALSO
        wctype(), iswalpha(), iswdigit(), iswspace()

    INTERNALS
        Internally matches `desc` against known property flags and applies
        simple mask-based tests.

******************************************************************************/
{
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();

    if (StdCBase->__locale_cur && StdCBase->__locale_cur->__lc_tbl_clsfy && (wc >= 0 && wc < StdCBase->__locale_cur->__lc_tbl_size))
        return (StdCBase->__locale_cur->__lc_tbl_clsfy[wc] & desc) != 0;
    return 0;
}
