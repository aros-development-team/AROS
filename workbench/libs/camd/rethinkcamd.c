/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/

#include <proto/exec.h>

#include "camd_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH0(LONG, RethinkCAMD,

/*  SYNOPSIS */

/*  LOCATION */
	struct CamdBase *, CamdBase, 36, Camd)

/*  FUNCTION
		Make camd reload midi preferences.

    INPUTS

    RESULT
		0 on success.

    NOTES

    EXAMPLE

    BUGS
		Not implemented.

    SEE ALSO

    INTERNALS

    HISTORY

	2001-01-12 ksvalast first created

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct CamdBase *,CamdBase)

	aros_print_not_implemented("RethinkCAMD");

	return 0;

   AROS_LIBFUNC_EXIT
}

