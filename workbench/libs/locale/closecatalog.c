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

	AROS_LH1(void, CloseCatalog,

/*  SYNOPSIS */
	AROS_LHA(struct Catalog *, catalog, A0),

/*  LOCATION */
	struct Library *, LocaleBase, 6, Locale)

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

    aros_print_not_implemented ("CloseCatalog");

    AROS_LIBFUNC_EXIT
} /* CloseCatalog */
