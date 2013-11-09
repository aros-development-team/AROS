/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/debug.h>
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

    struct DosInfo *di = BADDR(DOSBase->dl_Root->rn_Info);

    D(bug("UnLockDosList: flags = $%lx\n", flags));

    if (flags & LDF_ALL)
        ReleaseSemaphore(&di->di_DevLock);

    if (flags & LDF_ENTRY)
        ReleaseSemaphore(&di->di_EntryLock);

    if (flags & LDF_DELETE)
        ReleaseSemaphore(&di->di_DeleteLock);

/* This came from MorphOS. Left for reference.
    Permit(); */

    AROS_LIBFUNC_EXIT
} /* UnLockDosList */
