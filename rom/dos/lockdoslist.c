/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc:
*/
#include <aros/debug.h>
#include <proto/exec.h>
#include <stddef.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(struct DosList *, LockDosList,

/*  SYNOPSIS */
        AROS_LHA(ULONG, flags, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 109, Dos)

/*  FUNCTION
        Waits until the desired dos lists are free then gets a lock on them.
        A handle is returned that can be used for FindDosEntry().
        Calls to this function nest, i.e. you must call UnLockDosList()
        as often as you called LockDosList(). Always lock all lists
        at once - do not try to get a lock on one of them then on another.

    INPUTS
        flags - what lists to lock

    RESULT
        Handle to the dos list. This is not a direct pointer
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
    int i;

    D(bug("LockDosList: flags = $%lx\n", flags));

    if (((flags & (LDF_READ|LDF_WRITE)) != LDF_READ &&
         (flags & (LDF_READ|LDF_WRITE)) != LDF_WRITE) ||
        (flags & ~(LDF_READ|LDF_WRITE|LDF_DEVICES|LDF_VOLUMES|LDF_ASSIGNS|LDF_ENTRY|LDF_DELETE)))
        return NULL;

    for (i = 0; i < 3; i++)
    {
        if (flags & Dos_ListLockFlags[i])
        {
            if (flags & LDF_WRITE)
                ObtainSemaphore(&(&di->di_DevLock)[i]);
            else
                ObtainSemaphoreShared(&(&di->di_DevLock)[i]);
        }
    }

/* This strange thing came from MorphOS.
    Forbid(); */

    return (struct DosList *)&di->di_DevInfo;

    AROS_LIBFUNC_EXIT
} /* LockDosList */

const UBYTE Dos_ListLockFlags[3] = { LDF_ALL, LDF_ENTRY, LDF_DELETE };

/* LockDosList(), UnLockDosList() and AttemptLockDosList() reach the three
 * semaphores as an array starting at di_DevLock */
typedef char Dos_ListLocksAdjacent[
    (offsetof(struct DosInfo, di_EntryLock) == offsetof(struct DosInfo, di_DevLock) + sizeof(struct SignalSemaphore) &&
     offsetof(struct DosInfo, di_DeleteLock) == offsetof(struct DosInfo, di_DevLock) + 2 * sizeof(struct SignalSemaphore))
    ? 1 : -1];
