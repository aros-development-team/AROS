/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ConvToUpper() - stub for the language toupper() function.
    Lang: english
*/

#include <exec/types.h>
#include "locale_intern.h"
#include <aros/asmcall.h>

#define        DEBUG_CONVTOUPPER(x)        ;

/*****************************************************************************

    NAME */
#include <proto/locale.h>

        AROS_LH2(ULONG, ConvToUpper,

/*  SYNOPSIS */
        AROS_LHA(const struct Locale *, locale, A0),
        AROS_LHA(ULONG          , character, D0),

/*  LOCATION */
        struct LocaleBase *, LocaleBase, 9, Locale)

/*  FUNCTION
        ConvToUpper() will determine if a character is a lower case
        character and if so convert it to the upper case equivalent.
        Otherwise it will return the original character.

    INPUTS
        locale      - The Locale to use for this conversion or NULL for
                      the system default locale.
        character   - The character to convert.

    RESULT
        The possibly converted character.

    NOTES
        This function requires a full 32-bit character in order to support
        future multi-byte character sets.

    EXAMPLE

    BUGS

    SEE ALSO
        ConvToLower()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG retval;
    struct Locale *def_locale;

    if (locale == NULL)
    {
        locale = OpenLocale(NULL);
        def_locale = (struct Locale *)locale;
    }

    DEBUG_CONVTOUPPER(dprintf("ConvToUpper: locale 0x%lx char 0x%lx\n",
            locale, character));

    DEBUG_CONVTOUPPER(dprintf("ConvToUpper: func 0x%lx\n",
            IntL(locale)->il_LanguageFunctions[1]));

#ifdef AROS_CALL1
    retval = AROS_CALL1(ULONG, IntL(locale)->il_LanguageFunctions[1],
        AROS_LCA(ULONG, character, D0), struct LocaleBase *, LocaleBase);
#else
    retval = AROS_UFC2(ULONG, IntL(locale)->il_LanguageFunctions[1],
        AROS_UFCA(ULONG, character, D0),
        AROS_UFCA(struct LocaleBase *, LocaleBase, A6));
#endif

    DEBUG_CONVTOUPPER(dprintf("ConvToUpper: retval 0x%lx\n", retval));

    CloseLocale(def_locale);

    return retval;

    AROS_LIBFUNC_EXIT
}
