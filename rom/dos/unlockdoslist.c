/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc:
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
    int i;

    D(bug("UnLockDosList: flags = $%lx\n", flags));

    for (i = 0; i < 3; i++)
    {
        if (flags & Dos_ListLockFlags[i])
            ReleaseSemaphore(&(&di->di_DevLock)[i]);
    }

/* This came from MorphOS. Left for reference.
    Permit(); */

    AROS_LIBFUNC_EXIT
} /* UnLockDosList */
