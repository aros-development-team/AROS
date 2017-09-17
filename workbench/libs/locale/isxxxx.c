/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: IsXXXX() - Stub for Language isXXXXX() functions.
    Lang: english
*/
#include "locale_intern.h"
#include <aros/asmcall.h>
#include <proto/locale.h>

/*****************************************************************************

    NAME
#include <proto/locale.h>

        AROS_LH2(BOOL, IsXXXX,

    SYNOPSIS
        AROS_LHA(const struct Locale *,   locale, A0),
        AROS_LHA(ULONG,             character, D0),

    LOCATION
        struct LocaleBase *, LocaleBase, 0, Locale)

    FUNCTION
        These functions allow you to find out whether a character
        matches a certain type according to the current Locale
        settings.

        The functions available are:

        IsAlNum()  - is this an alphanumeric character
        IsAlpha()  - is this an alphabet character
        IsCntrl()  - is this a control character
        IsDigit()  - is this a decimal digit character
        IsGraph()  - is this a graphical character
        IsLower()  - is this a lowercase character
        IsPrint()  - is this a printable character
        IsPunct()  - is this a punctuation character
        IsSpace()  - is this a whitespace character
        IsUpper()  - is this an uppercase character
        IsXDigit() - is this a hexadecimal digit

    INPUTS
        locale      - The Locale to use for this function or NULL
                      for the system default locale.
        character   - the character to test

    RESULT
        ind - An indication of whether the character matches the type.
            TRUE - if the character is of the required type,
            FALSE - otherwise

    NOTES
        These functions require a 32-bit character to support future
        multi-byte character sets.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/


AROS_LH2(ULONG, IsAlNum,
    AROS_LHA(struct Locale *, locale, A0),
    AROS_LHA(ULONG          , character, D0),
    struct LocaleBase *, LocaleBase, 14, Locale)
{
    AROS_LIBFUNC_INIT

    ULONG retval;
    struct Locale *def_locale = NULL;
    
    if (locale == NULL)
    {
        locale = OpenLocale(NULL);
        def_locale = locale;
    }

#ifdef AROS_CALL1
    retval = AROS_CALL1(BOOL, IntL(locale)->il_LanguageFunctions[4],
        AROS_LCA(ULONG,    character, D0),
        struct LocaleBase *, LocaleBase);
#else
    retval = AROS_UFC2(ULONG, IntL(locale)->il_LanguageFunctions[4],
        AROS_UFCA(ULONG,    character, D0),
        AROS_UFCA(struct LocaleBase *, LocaleBase, A6));
#endif

    CloseLocale(def_locale);
    return retval;

    AROS_LIBFUNC_EXIT
} /* IsAlNum */

AROS_LH2(ULONG, IsAlpha,
    AROS_LHA(struct Locale *, locale, A0),
    AROS_LHA(ULONG          , character, D0),
    struct LocaleBase *, LocaleBase, 15, Locale)
{
    AROS_LIBFUNC_INIT

    ULONG retval;
    struct Locale *def_locale = NULL;
    
    if (locale == NULL)
    {
        locale = OpenLocale(NULL);
        def_locale = locale;
    }

#ifdef AROS_CALL1
    retval = AROS_CALL1(BOOL, IntL(locale)->il_LanguageFunctions[5],
        AROS_LCA(ULONG,    character, D0),
        struct LocaleBase *, LocaleBase);
#else
    retval = AROS_UFC2(ULONG, IntL(locale)->il_LanguageFunctions[5],
        AROS_UFCA(ULONG,    character, D0),
        AROS_UFCA(struct LocaleBase *, LocaleBase, A6));
#endif

    CloseLocale(def_locale);
    return retval;

    AROS_LIBFUNC_EXIT
} /* IsAlpha */

AROS_LH2(ULONG, IsCntrl,
    AROS_LHA(struct Locale *, locale, A0),
    AROS_LHA(ULONG          , character, D0),
    struct LocaleBase *, LocaleBase, 16, Locale)
{
    AROS_LIBFUNC_INIT

    ULONG retval;
    struct Locale *def_locale = NULL;
    
    if (locale == NULL)
    {
        locale = OpenLocale(NULL);
        def_locale = locale;
    }

#ifdef AROS_CALL1
    retval = AROS_CALL1(BOOL, IntL(locale)->il_LanguageFunctions[6],
        AROS_LCA(ULONG,    character, D0),
        struct LocaleBase *, LocaleBase);
#else
    retval = AROS_UFC2(ULONG, IntL(locale)->il_LanguageFunctions[6],
        AROS_UFCA(ULONG,    character, D0),
        AROS_UFCA(struct LocaleBase *, LocaleBase, A6));
#endif

    CloseLocale(def_locale);
    return retval;

    AROS_LIBFUNC_EXIT
} /* IsCntrl */

AROS_LH2(ULONG, IsDigit,
    AROS_LHA(struct Locale *, locale, A0),
    AROS_LHA(ULONG          , character, D0),
    struct LocaleBase *, LocaleBase, 17, Locale)
{
    AROS_LIBFUNC_INIT

    ULONG retval;
    struct Locale *def_locale = NULL;
    
    if (locale == NULL)
    {
        locale = OpenLocale(NULL);
        def_locale = locale;
    }

#ifdef AROS_CALL1
    retval = AROS_CALL1(BOOL, IntL(locale)->il_LanguageFunctions[7],
        AROS_LCA(ULONG,    character, D0),
        struct LocaleBase *, LocaleBase);
#else
    retval = AROS_UFC2(ULONG, IntL(locale)->il_LanguageFunctions[7],
        AROS_UFCA(ULONG,    character, D0),
        AROS_UFCA(struct LocaleBase *, LocaleBase, A6));
#endif

    CloseLocale(def_locale);
    return retval;

    AROS_LIBFUNC_EXIT
} /* IsDigit */

AROS_LH2(ULONG, IsGraph,
    AROS_LHA(struct Locale *, locale, A0),
    AROS_LHA(ULONG          , character, D0),
    struct LocaleBase *, LocaleBase, 18, Locale)
{
    AROS_LIBFUNC_INIT

    ULONG retval;
    struct Locale *def_locale = NULL;
    
    if (locale == NULL)
    {
        locale = OpenLocale(NULL);
        def_locale = locale;
    }

#ifdef AROS_CALL1
    retval = AROS_CALL1(BOOL, IntL(locale)->il_LanguageFunctions[8],
        AROS_LCA(ULONG,    character, D0),
        struct LocaleBase *, LocaleBase);
#else
    retval = AROS_UFC2(ULONG, IntL(locale)->il_LanguageFunctions[8],
        AROS_UFCA(ULONG,    character, D0),
        AROS_UFCA(struct LocaleBase *, LocaleBase, A6));
#endif

    CloseLocale(def_locale);
    return retval;

    AROS_LIBFUNC_EXIT
} /* IsGraph */

AROS_LH2(ULONG, IsLower,
    AROS_LHA(struct Locale *, locale, A0),
    AROS_LHA(ULONG          , character, D0),
    struct LocaleBase *, LocaleBase, 19, Locale)
{
    AROS_LIBFUNC_INIT

    ULONG retval;
    struct Locale *def_locale = NULL;
    
    if (locale == NULL)
    {
        locale = OpenLocale(NULL);
        def_locale = locale;
    }

#ifdef AROS_CALL1
    retval = AROS_CALL1(BOOL, IntL(locale)->il_LanguageFunctions[9],
        AROS_LCA(ULONG,    character, D0),
        struct LocaleBase *, LocaleBase);
#else
    retval = AROS_UFC2(ULONG, IntL(locale)->il_LanguageFunctions[9],
        AROS_UFCA(ULONG,    character, D0),
        AROS_UFCA(struct LocaleBase *, LocaleBase, A6));
#endif

    CloseLocale(def_locale);
    return retval;

    AROS_LIBFUNC_EXIT
} /* IsLower */

AROS_LH2(ULONG, IsPrint,
    AROS_LHA(struct Locale *, locale, A0),
    AROS_LHA(ULONG          , character, D0),
    struct LocaleBase *, LocaleBase, 20, Locale)
{
    AROS_LIBFUNC_INIT

    ULONG retval;
    struct Locale *def_locale = NULL;
    
    if (locale == NULL)
    {
        locale = OpenLocale(NULL);
        def_locale = locale;
    }

#ifdef AROS_CALL1
    retval = AROS_CALL1(BOOL, IntL(locale)->il_LanguageFunctions[10],
        AROS_LCA(ULONG,    character, D0),
        struct LocaleBase *, LocaleBase);
#else
    retval = AROS_UFC2(ULONG, IntL(locale)->il_LanguageFunctions[10],
        AROS_UFCA(ULONG,    character, D0),
        AROS_UFCA(struct LocaleBase *, LocaleBase, A6));
#endif

    CloseLocale(def_locale);
    return retval;

    AROS_LIBFUNC_EXIT
} /* IsPrint */

AROS_LH2(ULONG, IsPunct,
    AROS_LHA(struct Locale *, locale, A0),
    AROS_LHA(ULONG          , character, D0),
    struct LocaleBase *, LocaleBase, 21, Locale)
{
    AROS_LIBFUNC_INIT

    ULONG retval;
    struct Locale *def_locale = NULL;
    
    if (locale == NULL)
    {
        locale = OpenLocale(NULL);
        def_locale = locale;
    }

#ifdef AROS_CALL1
    retval = AROS_CALL1(BOOL, IntL(locale)->il_LanguageFunctions[11],
        AROS_LCA(ULONG,    character, D0),
        struct LocaleBase *, LocaleBase);
#else
    retval = AROS_UFC2(ULONG, IntL(locale)->il_LanguageFunctions[11],
        AROS_UFCA(ULONG,    character, D0),
        AROS_UFCA(struct LocaleBase *, LocaleBase, A6));
#endif

    CloseLocale(def_locale);
    return retval;

    AROS_LIBFUNC_EXIT
} /* IsPunct */

AROS_LH2(ULONG, IsSpace,
    AROS_LHA(struct Locale *, locale, A0),
    AROS_LHA(ULONG          , character, D0),
    struct LocaleBase *, LocaleBase, 22, Locale)
{
    AROS_LIBFUNC_INIT

    ULONG retval;
    struct Locale *def_locale = NULL;
    
    if (locale == NULL)
    {
        locale = OpenLocale(NULL);
        def_locale = locale;
    }

#ifdef AROS_CALL1
    retval = AROS_CALL1(BOOL, IntL(locale)->il_LanguageFunctions[12],
        AROS_LCA(ULONG,    character, D0),
        struct LocaleBase *, LocaleBase);
#else
    retval = AROS_UFC2(ULONG, IntL(locale)->il_LanguageFunctions[12],
        AROS_UFCA(ULONG,    character, D0),
        AROS_UFCA(struct LocaleBase *, LocaleBase, A6));
#endif

    CloseLocale(def_locale);
    return retval;

    AROS_LIBFUNC_EXIT
} /* IsSpace */

AROS_LH2(ULONG, IsUpper,
    AROS_LHA(struct Locale *, locale, A0),
    AROS_LHA(ULONG          , character, D0),
    struct LocaleBase *, LocaleBase, 23, Locale)
{
    AROS_LIBFUNC_INIT

    ULONG retval;
    struct Locale *def_locale = NULL;
    
    if (locale == NULL)
    {
        locale = OpenLocale(NULL);
        def_locale = locale;
    }

#ifdef AROS_CALL1
    retval = AROS_CALL1(BOOL, IntL(locale)->il_LanguageFunctions[13],
        AROS_LCA(ULONG,    character, D0),
        struct LocaleBase *, LocaleBase);
#else
    retval = AROS_UFC2(ULONG, IntL(locale)->il_LanguageFunctions[13],
        AROS_UFCA(ULONG,    character, D0),
        AROS_UFCA(struct LocaleBase *, LocaleBase, A6));
#endif

    CloseLocale(def_locale);
    return retval;

    AROS_LIBFUNC_EXIT
} /* IsUpper */

AROS_LH2(ULONG, IsXDigit,
    AROS_LHA(struct Locale *, locale, A0),
    AROS_LHA(ULONG          , character, D0),
    struct LocaleBase *, LocaleBase, 24, Locale)
{
    AROS_LIBFUNC_INIT

    ULONG retval;
    struct Locale *def_locale = NULL;
    
    if (locale == NULL)
    {
        locale = OpenLocale(NULL);
        def_locale = locale;
    }

#ifdef AROS_CALL1
    retval = AROS_CALL1(BOOL, IntL(locale)->il_LanguageFunctions[14],
        AROS_LCA(ULONG,    character, D0),
        struct LocaleBase *, LocaleBase);
#else
    retval = AROS_UFC2(ULONG, IntL(locale)->il_LanguageFunctions[14],
        AROS_UFCA(ULONG,    character, D0),
        AROS_UFCA(struct LocaleBase *, LocaleBase, A6));
#endif

    CloseLocale(def_locale);
    return retval;

    AROS_LIBFUNC_EXIT
} /* IsXDigit */
