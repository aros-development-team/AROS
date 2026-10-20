/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include <langinfo.h>  // for nl_item, CODESET, ABDAY_1, D_FMT, etc.
#include <locale.h>    // for setlocale, localeconv
#include <string.h>    // for strchr
#include <time.h>      // for strftime, struct tm, time
#include <stdlib.h>    // for NULL
#include <langinfo.h>

#include <aros/types/locale_s.h>

/****************************************************************************
    NAME
        char * nl_langinfo_l(

    SYNOPSIS
        nl_item item, locale_t loc)

    FUNCTION
        Call nl_langinfo() while attempting a temporary C-library
        locale change.

    INPUTS
        item - Item accepted by nl_langinfo().
        loc - Readable locale object, normally from newlocale(),
              or NULL to skip the requested switch. LC_GLOBAL_LOCALE
              is not supported and must not be passed.

    RESULT
        The borrowed result of nl_langinfo(), or an empty string
        if saving the locale name fails. Do not modify or free it.
        The buffer-sharing rules and result defects of nl_langinfo()
        apply. Empty results do not identify a unique error.

    NOTES
        Include <langinfo.h>. The function allocates a temporary
        copy of a locale name and frees it before returning. It
        does not retain loc. Calls to Forbid() and Permit() bracket
        the operation; they do not make the returned buffers private.
        Use ordinary task context and synchronize shared use.

    BUGS
        The initial setlocale(LC_ALL, NULL) resets the C-library
        locale to "C" instead of querying it. The saved name is
        therefore "C", and the previous locale is lost, including
        on allocation failure. A normal return restores "C".

        Locale-change failures are ignored. Weekday and AM/PM
        queries use the default system locale, not loc; CODESET
        resets the C-library locale again. This function does not
        provide independent locale-object queries. The formatting
        and resource defects listed for nl_langinfo() also apply.

    SEE ALSO
        nl_langinfo(), newlocale(), setlocale()

*****************************************************************************/
char *nl_langinfo_l(nl_item item, locale_t loc) {
    const char *original;
    char *saved_locale = NULL;

    Forbid();
    original = setlocale(LC_ALL, NULL);
    if (original) {
        // Duplicate current locale string to restore later
        saved_locale = strdup(original);
        if (!saved_locale) {
            Permit();
            return "";  // Allocation failed
        }
    }

    if (loc) {
        setlocale(LC_ALL, loc->__lc_name);  // Temporarily switch to requested locale
    }

    char *result = nl_langinfo(item);

    if (saved_locale) {
        setlocale(LC_ALL, saved_locale);  // Restore previous locale
        free(saved_locale);
    }
    Permit();

    return result;
}
