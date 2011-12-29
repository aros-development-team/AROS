/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ConvToLower() - Stub for the Language tolower() function.
    Lang: english
*/
#include <exec/types.h>
#include "locale_intern.h"
#include <aros/asmcall.h>

#define        DEBUG_CONVTOLOWER(x)        ;

/*****************************************************************************

    NAME */
#include <proto/locale.h>

    AROS_LH2(ULONG, ConvToLower,

/*  SYNOPSIS */
        AROS_LHA(const struct Locale *, locale, A0),
        AROS_LHA(ULONG          , character, D0),

/*  LOCATION */
        struct LocaleBase *, LocaleBase, 8, Locale)

/*  FUNCTION
        This function determine if the character supplied is upper case,
        and if it is, the character will be converted to lower case.
        Otherwise, the original character will be returned.

    INPUTS
        locale      - The Locale to use for this conversion.
        character   - The character to convert to lower case.

    RESULT
        The possibly converted character.

    NOTES
        This function requires a full 32-bit character in order to
        support future multi-byte character sets.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG retval;

    DEBUG_CONVTOLOWER(dprintf("ConvToLower: locale 0x%lx char 0x%lx\n",
            locale, character));

    DEBUG_CONVTOLOWER(dprintf("ConvToLower: func 0x%lx\n",
            IntL(locale)->il_LanguageFunctions[0]));

#ifdef AROS_CALL1
    retval = AROS_CALL1(ULONG, IntL(locale)->il_LanguageFunctions[0],
        AROS_LCA(ULONG, character, D0), struct LocaleBase *, LocaleBase);
#else
    retval = AROS_UFC2(ULONG, IntL(locale)->il_LanguageFunctions[0],
        AROS_UFCA(ULONG, character, D0),
        AROS_UFCA(struct LocaleBase *, LocaleBase, A6));
#endif

    DEBUG_CONVTOLOWER(dprintf("ConvToLower: retval 0x%lx\n", retval));

    return (retval);

    AROS_LIBFUNC_EXIT
}
