/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function towctrans().
*/

#include <aros/debug.h>

#include <aros/types/wctrans_t.h>
#include <aros/types/wint_t.h>

#define STDC_NOINLINE_WCTYPE

/*****************************************************************************

    NAME */
#include <wctype.h>

        wint_t towctrans(

/*  SYNOPSIS */
        wint_t wc, wctrans_t desc)

/*  FUNCTION
        Transforms a wide character according to the mapping identified by
        the wctrans_t descriptor.

    INPUTS
        wc    - the wide character to transform.
        desc  - transformation descriptor from `wctrans()`.

    RESULT
        The transformed wide character, or the original character if the
        descriptor is invalid.

    NOTES

    EXAMPLE
        wctrans_t t = wctrans("tolower");
        wint_t lc = towctrans(L'Z', t);  // 'z'

    BUGS
        Behavior is undefined if `desc` was not obtained from `wctrans()`.

    SEE ALSO
        wctrans(), towlower(), towupper()

    INTERNALS

******************************************************************************/
{
    if (!desc || !desc->func)
        return wc;
    return desc->func(wc);
}
