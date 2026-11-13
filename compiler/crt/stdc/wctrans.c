/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function wctrans().
*/

#include <aros/debug.h>

#include <aros/types/wctrans_t.h>

#include <string.h>

#define STDC_NOINLINE_WCTYPE

#include "__stdc_intbase.h"

/*****************************************************************************

    NAME */
#include <wctype.h>

        wctrans_t wctrans(

/*  SYNOPSIS */
        const char *prop)

/*  FUNCTION
        Returns a value representing a wide character mapping function,
        such as "tolower" or "toupper", to be used with `towctrans()`.

    INPUTS
        prop - a string specifying the mapping ("tolower", "toupper").

    RESULT
        A wctrans_t descriptor for the transformation, or 0 if not recognized.

    NOTES

    EXAMPLE
        wctrans_t map = wctrans("toupper");
        wchar_t upper = towctrans(L'a', map);  // 'A'

    BUGS

    SEE ALSO
        towctrans(), towupper(), towlower()

    INTERNALS

******************************************************************************/
{
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();

	if (StdCBase->__locale_cur && StdCBase->__locale_cur->__lc_wctrans_list)
		for (int i = 0; StdCBase->__locale_cur->__lc_wctrans_list[i].name; ++i) {
			if (strcmp(prop, StdCBase->__locale_cur->__lc_wctrans_list[i].name) == 0)
				return (wctrans_t)&StdCBase->__locale_cur->__lc_wctrans_list[i];
		}
    return NULL;
}
