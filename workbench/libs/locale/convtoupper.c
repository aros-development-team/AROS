/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ConvToUpper() - stub for the language toupper() function.
    Lang: english
*/

#include <exec/types.h>
#include "locale_intern.h"
#include <aros/asmcall.h>

/*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_LH2(ULONG, ConvToUpper,

/*  SYNOPSIS */
	AROS_LHA(struct Locale *, locale, A0),
	AROS_LHA(ULONG          , character, D0),

/*  LOCATION */
	struct LocaleBase *, LocaleBase, 9, Locale)

/*  FUNCTION
	ConvToUpper() will determine if a character is a lower case
	character and if so convert it to the upper case equivalent.
	Otherwise it will return the original character.

    INPUTS
	locale      - The Locale to use for this conversion.
	character   - The character to convert.

    RESULT
	The possibly converted character.

    NOTES
	This function requires a full 32-bit character in order to support
	future multi-byte character sets.

    EXAMPLE

    BUGS

    SEE ALSO
	ConvToLower

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    locale_lib.fd and clib/locale_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct LocaleBase *,LocaleBase)

    return AROS_CALL1(ULONG, IntL(locale)->il_LanguageFunctions[1],
	AROS_LCA(ULONG, character, D0),
	struct LocaleBase *, LocaleBase);

    AROS_LIBFUNC_EXIT
} /* ConvToUpper */
