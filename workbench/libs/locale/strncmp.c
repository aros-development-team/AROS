/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: StrnCmp() - Stub for the Locale StrnCmp() function.
    Lang: english
*/
#include <exec/types.h>
#include <proto/exec.h>
#include "locale_intern.h"
#include <aros/asmcall.h>

#define        DEBUG_STRNCMP(x)        ;

/*****************************************************************************

    NAME */
#include <proto/locale.h>

        AROS_LH5(LONG, StrnCmp,

/*  SYNOPSIS */
        AROS_LHA(const struct Locale *, locale, A0),
        AROS_LHA(CONST_STRPTR   , string1, A1),
        AROS_LHA(CONST_STRPTR   , string2, A2),
        AROS_LHA(LONG           , length, D0),
        AROS_LHA(ULONG          , type, D1),

/*  LOCATION */
        struct LocaleBase *, LocaleBase, 30, Locale)

/*  FUNCTION
        StrnCmp() will compare two strings, up to a maximum length
        of length using a specific kind of collation information
        according to the locale.

        The result will be less than zero, zero, or greater than zero
        depending upon whether the string string1 is less than, equal
        to, or greater than the string pointed to string2.

    INPUTS
        locale      -   Which locale to use for this comparison or
                        NULL for the system default locale.
        string1     -   NULL terminated string.
        string2     -   NULL terminated string.
        length      -   Maximum length of string to compare, or -1 to
            compare entire strings.
        type        -   How to compare the strings, values are:

            SC_ASCII
                Perform a simple ASCII case-insensitive comparison.
                This is the fastest comparison, but considers that
                accented characters are different to non-accented
                characters.

            SC_COLLATE1
                This sorts using the "primary sorting order". This
                means that characters such as 'e' and 'é' will be
                considered the same. This method also ignores
                case.

            SC_COLLATE2
                This will sort using both the primary and secondary
                sorting order. This is the slowest sorting method
                and should be used when presenting data to a user.

                The first pass is the same as SC_COLLATE1, meaning
                that two strings such as "role" and "rôle" would
                be sorted identically. The second pass will
                compare the diacritical marks.

    RESULT
        The relationship between the two strings.

            < 0 means   string1 < string2
            = 0 means   string1 == string2
            > 0 means   string1 > string2

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        OpenLocale(), CloseLocale(), StrConvert()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG result;
    struct Locale *def_locale = NULL;

    if (locale == NULL)
    {
        locale = OpenLocale(NULL);
        def_locale = (struct Locale *)locale;
    }

    DEBUG_STRNCMP(dprintf
        ("StrnCmp: locale 0x%lx <%s> <%s> len %ld type 0x%lx\n", locale,
            string1, string2, length, type));

    DEBUG_STRNCMP(dprintf("StrnCmp: Function 0x%lx\n",
            IntL(locale)->il_LanguageFunctions[16]));

#ifdef AROS_CALL4
    result = AROS_CALL4(ULONG, IntL(locale)->il_LanguageFunctions[16],
        AROS_LCA(CONST_STRPTR, string1, A1),
        AROS_LCA(CONST_STRPTR, string2, A2),
        AROS_LCA(ULONG, length, D0),
        AROS_LCA(ULONG, type, D1), struct LocaleBase *, LocaleBase);
#else
    result = AROS_UFC4(ULONG, IntL(locale)->il_LanguageFunctions[16],
        AROS_UFCA(CONST_STRPTR, string1, A1),
        AROS_UFCA(CONST_STRPTR, string2, A2),
        AROS_UFCA(ULONG, length, D0), AROS_UFCA(ULONG, type, D1));
#endif

    CloseLocale(def_locale);

    DEBUG_STRNCMP(dprintf("StrnCmp: retval %ld\n", result));

    return result;

    AROS_LIBFUNC_EXIT
}
