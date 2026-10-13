/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    Desc: POSIX newlocale() / freelocale() for the locales stdc knows.
*/
#include <aros/debug.h>

#include <errno.h>
#include <locale.h>
#include <string.h>
#define STDC_NOINLINE_WCTYPE
#include <wctype.h>
#include <aros/types/locale_s.h>

/* stdc's classification/case tables, shared by every locale this libc can
 * name. Included rather than exported: stdc.library keeps its own copy in
 * __locale.c and exposes no accessor for the objects, and a new LVO just to
 * hand out a pointer would widen the ABI for nothing. The tables are const. */
#include "../stdc/_unicode_tables.c"

static const struct __wctrans posixc_wctrans_list[] = {
    { "tolower", towlower },
    { "toupper", towupper },
    { NULL, NULL }
};

/* Mirrors stdc/__locale.c __locale_C / __locale_UTF8 field for field. */
static struct __locale posixc_locale_C = {
    .__lc_name = "C",
    .__lc_mb_max = 1,
#if __WCHAR_MAX__ > 255
    .__lc_tbl_size = 256,
#else
    .__lc_tbl_size = 128,
#endif
    .__lc_tbl_clsfy = unicode_wctype,
    .__lc_tbl_u2l = unicode_u2l,
    .__lc_tbl_l2u = unicode_l2u,
    .__lc_wctrans_list = posixc_wctrans_list
};

#if __WCHAR_MAX__ > 255
static struct __locale posixc_locale_UTF8 = {
    .__lc_name = "C.UTF-8",
    .__lc_mb_max = 4,
    .__lc_tbl_size = 256,
    .__lc_tbl_clsfy = unicode_wctype,
    .__lc_tbl_u2l = unicode_u2l,
    .__lc_tbl_l2u = unicode_l2u,
    .__lc_wctrans_list = posixc_wctrans_list
};
#endif

/*****************************************************************************

    NAME */
#include <locale.h>

        locale_t newlocale(

/*  SYNOPSIS */
        int category_mask,
        const char *locale_name,
        locale_t base)

/*  FUNCTION
        Create a locale object for locale_name (POSIX.1-2008). AROS's libc
        knows the "C" (= "POSIX", = "") locale and, with wide wchar_t,
        "C.UTF-8"; those are returned as shared, immutable objects.

    INPUTS
        category_mask - LC_*_MASK bits to take from locale_name; every
                        category comes from the same table here, so the
                        mask only has to be valid.
        locale_name   - "C", "POSIX", "" (the default), "C.UTF-8".
        base          - modified and returned unchanged when non-NULL and
                        not LC_GLOBAL_LOCALE (the objects are shared, so
                        "modifying" the C locale into the C locale is the
                        identity).

    RESULT
        The locale object, or NULL with errno set: EINVAL for a bad mask or
        LC_GLOBAL_LOCALE base, ENOENT for an unknown locale name.

    NOTES
        Returning NULL for "C" (the previous stub) broke every consumer
        that builds its C-locale handle at startup — libc++ does exactly
        that (newlocale(LC_ALL_MASK, "C", 0)) and then calls islower_l()/
        toupper_l() with it, which dereference the NULL locale: every
        std::ostream << double crashed on AROS.

    SEE ALSO
        freelocale(), uselocale(), setlocale()

******************************************************************************/
{
    if (category_mask & ~LC_ALL_MASK)
    {
        errno = EINVAL;
        return NULL;
    }
    if (base == LC_GLOBAL_LOCALE)
    {
        errno = EINVAL;
        return NULL;
    }
    if (locale_name == NULL)
    {
        errno = EINVAL;
        return NULL;
    }

    if (locale_name[0] == '\0' || strcmp(locale_name, "C") == 0 ||
        strcmp(locale_name, "POSIX") == 0)
        return &posixc_locale_C;
#if __WCHAR_MAX__ > 255
    if (strcmp(locale_name, "C.UTF-8") == 0 || strcmp(locale_name, "C.utf8") == 0)
        return &posixc_locale_UTF8;
#endif

    errno = ENOENT;
    return NULL;
}
