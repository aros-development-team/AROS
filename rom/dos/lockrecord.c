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

	AROS_LH5(BOOL, LockRecord,

/*  SYNOPSIS */
	AROS_LHA(BPTR , fh, D1),
	AROS_LHA(ULONG, offset, D2),
	AROS_LHA(ULONG, length, D3),
	AROS_LHA(ULONG, mode, D4),
	AROS_LHA(ULONG, timeout, D5),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 45, Dos)

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

    aros_print_not_implemented ("LockRecord");

    return DOSFALSE;
    AROS_LIBFUNC_EXIT
} /* LockRecord */
