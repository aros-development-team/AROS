/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include "locale_intern.h"

/*****************************************************************************

    NAME */
#include <clib/locale_protos.h>

	AROS_LH3(STRPTR, GetCatalogStr,

/*  SYNOPSIS */
	AROS_LHA(struct Catalog *, catalog, A0),
	AROS_LHA(LONG            , stringNum, D0),
	AROS_LHA(STRPTR          , defaultString, A1),

/*  LOCATION */
	struct Library *, LocaleBase, 12, Locale)

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

    aros_print_not_implemented ("GetCatalogStr");

    AROS_LIBFUNC_EXIT
} /* GetCatalogStr */
