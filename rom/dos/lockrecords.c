/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <dos/record.h>
#include <clib/dos_protos.h>

	AROS_LH2(BOOL, LockRecords,

/*  SYNOPSIS */
	AROS_LHA(struct RecordLock *, recArray, D1),
	AROS_LHA(ULONG              , timeout, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 46, Dos)

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

    aros_print_not_implemented ("LockRecords");

    return DOSFALSE;
    AROS_LIBFUNC_EXIT
} /* LockRecords */
