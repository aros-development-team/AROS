/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/10/24 15:50:38  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/08/13 13:52:52  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.2  1996/08/01 17:40:59  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

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
