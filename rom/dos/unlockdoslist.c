/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(void, UnLockDosList,

/*  SYNOPSIS */
	AROS_LHA(ULONG, flags, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 110, Dos)

/*  FUNCTION
	Frees a lock on the dos lists given by LockDosList().

    INPUTS
	flags - the same value as given to LockDosList().

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
    ReleaseSemaphore(&DOSBase->dl_DosListLock);
    AROS_LIBFUNC_EXIT
} /* UnLockDosList */
