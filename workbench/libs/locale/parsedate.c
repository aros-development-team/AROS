/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "locale_intern.h"

/*****************************************************************************

    NAME */
#include <clib/locale_protos.h>

	AROS_LH4(BOOL, ParseDate,

/*  SYNOPSIS */
	AROS_LHA(struct Locale    *, locale, A0),
	AROS_LHA(struct DateStamp *, date, A1),
	AROS_LHA(STRPTR            , fmtTemplate, A2),
	AROS_LHA(struct Hook      *, getCharFunc, A3),

/*  LOCATION */
	struct Library *, LocaleBase, 27, Locale)

/*  FUNCTION

    INPUTS

    RESULT

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
    AROS_LIBBASE_EXT_DECL(struct Library *,LocaleBase)
    extern void aros_print_not_implemented (char *);

    aros_print_not_implemented ("ParseDate");

    AROS_LIBFUNC_EXIT
} /* ParseDate */
