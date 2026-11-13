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

        AROS_LH1(struct DosList *, AttemptLockDosList,

/*  SYNOPSIS */
        AROS_LHA(ULONG, flags, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 111, Dos)

/*  FUNCTION
        Tries to get a lock on some of the dos lists. If all went
        well a handle is returned that can be used for FindDosEntry().
        Don't try to busy wait until the lock can be granted - use
        LockDosList() instead.

    INPUTS
        flags  --  what lists to lock

    RESULT
        Handle to the dos list or NULL. This is not a direct pointer
        to the first list element but to a pseudo element instead.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct DosInfo *di = BADDR(DOSBase->dl_Root->rn_Info);
    struct DosList *dl = (struct DosList *)&di->di_DevInfo;
    ULONG got = 0;
    int i;

    D(bug("AttemptLockDosList: flags = $%lx\n", flags));

    if (((flags & (LDF_READ|LDF_WRITE)) != LDF_READ &&
         (flags & (LDF_READ|LDF_WRITE)) != LDF_WRITE) ||
        (flags & ~(LDF_READ|LDF_WRITE|LDF_DEVICES|LDF_VOLUMES|LDF_ASSIGNS|LDF_ENTRY|LDF_DELETE)))
        return NULL;

    for (i = 0; i < 3; i++)
    {
        if (flags & Dos_ListLockFlags[i])
        {
            struct SignalSemaphore *sem = &(&di->di_DevLock)[i];

            if (!((flags & LDF_WRITE) ? AttemptSemaphore(sem) : AttemptSemaphoreShared(sem)))
                dl = NULL;
            else
                got |= 1 << i;
        }
    }

/* This came from MorphOS source code, however looks strange.
   Commented out but left for reference.
    if (dl)
        Forbid(); */
    if (!dl) {
        for (i = 0; i < 3; i++)
        {
            if (got & (1 << i))
                ReleaseSemaphore(&(&di->di_DevLock)[i]);
        }
    }

    D(bug("AttemptLockDosList: result = $%lx\n", dl));

    return dl;

    AROS_LIBFUNC_EXIT
} /* AttemptLockDosList */
