/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    ReleaseSemaphore(&DOSBase->dl_DosListLock);
    AROS_LIBFUNC_EXIT
} /* UnLockDosList */
