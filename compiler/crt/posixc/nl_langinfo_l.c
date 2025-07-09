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
