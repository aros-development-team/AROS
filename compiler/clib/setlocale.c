/*
    Copyright © 2003-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function localeconv().
*/

#include <locale.h>
#include <string.h>

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
        locale - string representing "C"

    RESULT
        The lconv struct

    NOTES
        arosstdc.library only support "C" locale. So only NULL or
        "C" are accepted for locale and this function does not
        have an effect.

    EXAMPLE

    BUGS

    SEE ALSO
        locale.h

    INTERNALS

******************************************************************************/
{
    if (category < LC_ALL || category > LC_TIME)
        return NULL;

    if (locale == NULL || strcmp(locale, "C") == 0)
        return "C";
    
    return NULL;
}
