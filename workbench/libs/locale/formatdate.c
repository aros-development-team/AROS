/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <exec/types.h>
#include <utility/hooks.h>
#include <proto/utility.h>
#include "locale_intern.h"

/*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_LH4(void, FormatDate,

/*  SYNOPSIS */
	AROS_LHA(struct Locale    *, locale, A0),
	AROS_LHA(STRPTR            , fmtTemplate, A1),
	AROS_LHA(struct DateStamp *, date, A2),
	AROS_LHA(struct Hook      *, putCharFunc, A3),

/*  LOCATION */
	struct Locale *, LocaleBase, 10, Locale)

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

    aros_print_not_implemented ("FormatDate");

    AROS_LIBFUNC_EXIT
} /* FormatDate */
