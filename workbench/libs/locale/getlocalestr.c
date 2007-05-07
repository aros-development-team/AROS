/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GetLocaleStr() - Get a builtin system string.
    Lang: english
*/
#include <exec/types.h>
#include <proto/exec.h>
#include "locale_intern.h"
#include <aros/asmcall.h>

/*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_LH2(STRPTR, GetLocaleStr,

/*  SYNOPSIS */
	AROS_LHA(struct Locale *, locale, A0),
	AROS_LHA(ULONG          , stringNum, D0),

/*  LOCATION */
	struct LocaleBase *, LocaleBase, 13, Locale)

/*  FUNCTION
	This function will return a system standard string from
	the current Locale.

    INPUTS
	locale      - The current locale.
	stringNum   - The number of the string to get a pointer to.
		      See the include file <libraries/locale.h>
		      for a list of possible values.

    RESULT
	A pointer to a NULL-terminated string, or NULL if the string
	requested was unknown. The returned string is READ-ONLY and
	is valid only as long as the Locale remains open.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    locale_lib.fd and clib/locale_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

#ifdef AROS_CALL1
    return AROS_CALL1(STRPTR, IntL(locale)->il_LanguageFunctions[3],
	AROS_LCA(ULONG, stringNum, D0),
	struct LocaleBase *, LocaleBase);
#else
    return AROS_UFC2(STRPTR, IntL(locale)->il_LanguageFunctions[3],
	AROS_UFCA(ULONG, stringNum, D0),
	AROS_UFCA(struct LocaleBase *, LocaleBase, A6));
#endif

    AROS_LIBFUNC_EXIT
} /* GetLocaleStr */
