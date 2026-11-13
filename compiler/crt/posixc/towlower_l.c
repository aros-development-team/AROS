/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function towlower_l().
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

        wint_t towlower_l(

/*  SYNOPSIS */
        wint_t wc,
        locale_t locale)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        towupper(), towctrans()

    INTERNALS

******************************************************************************/
{
    return towlower(wc);
}
