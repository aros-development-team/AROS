/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <clib/dos_protos.h>

	AROS_LH4(BPTR, InternalLoadSeg,

/*  SYNOPSIS */
	AROS_LHA(BPTR  , fh, D0),
	AROS_LHA(BPTR  , table, A0),
	AROS_LHA(LONG *, funcarray, A1),
	AROS_LHA(LONG *, stack, A2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 126, Dos)

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
    extern void aros_print_not_implemented (char *);

    aros_print_not_implemented ("InternalLoadSeg");

    return DOSFALSE;
    AROS_LIBFUNC_EXIT
} /* InternalLoadSeg */
