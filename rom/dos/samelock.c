/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(LONG, SameLock,

/*  SYNOPSIS */
	AROS_LHA(BPTR, lock1, D1),
	AROS_LHA(BPTR, lock2, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 70, Dos)

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
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

#warning TODO: Write dos/SameLock()
    aros_print_not_implemented ("SameLock");

    return 0;
    AROS_LIBFUNC_EXIT
} /* SameLock */
