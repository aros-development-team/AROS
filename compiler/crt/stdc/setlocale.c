/*
    Copyright (C) 2003-2025, The AROS Development Team. All rights reserved.

    C99 function localeconv().
*/

#include <locale.h>
#include <string.h>
#include <aros/types/locale_s.h>

#include "__locale.h"

/*****************************************************************************

    NAME */
#include <string.h>

        char *setlocale(

/*  SYNOPSIS */
        int category,
        const char *locale)

/*  FUNCTION

    INPUTS
        category - category as defined in locale.h
        locale - string representing the desired locale (see notes)

    RESULT
        The lconv struct

    NOTES
        stdc.library supports "C" (and if the compiler has been
        compiled with __WCHAR_MAX__ > 255, "UTF8") locale(s).
        Currently accepted values for locale are "C", and "C.UTF-8".

    EXAMPLE

    BUGS

    SEE ALSO
        locale.h

    INTERNALS

******************************************************************************/
{
    locale_t loc;

    if (category < LC_ALL || category > LC_TIME)
        return NULL;

    loc = __get_setlocale_internal(locale);
    return loc ? (char *)loc->__lc_name : NULL;
}
