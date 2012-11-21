/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
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
    ULONG DevSem = FALSE, EntrySem = FALSE, DelSem = FALSE;

    D(bug("AttemptLockDosList: flags = $%lx\n", flags));

    if (((flags & (LDF_READ|LDF_WRITE)) != LDF_READ &&
         (flags & (LDF_READ|LDF_WRITE)) != LDF_WRITE) ||
        (flags & ~(LDF_READ|LDF_WRITE|LDF_DEVICES|LDF_VOLUMES|LDF_ASSIGNS|LDF_ENTRY|LDF_DELETE)))
        return NULL;

    if (flags & LDF_ALL)
    {
        if (flags & LDF_WRITE)
            DevSem = AttemptSemaphore(&di->di_DevLock);
        else
            DevSem = AttemptSemaphoreShared(&di->di_DevLock);
        if (!DevSem)
            dl = NULL;
    }

    if (flags & LDF_ENTRY)
    {
        if (flags & LDF_WRITE)
            EntrySem = AttemptSemaphore(&di->di_EntryLock);
        else
            EntrySem = AttemptSemaphoreShared(&di->di_EntryLock);
        if (!EntrySem)
            dl = NULL;
    }

    if (flags & LDF_DELETE)
    {
        if (flags & LDF_WRITE)
            DelSem = AttemptSemaphore(&di->di_DeleteLock);
        else
            DelSem = AttemptSemaphoreShared(&di->di_DeleteLock);
        if (!DelSem)
            dl = NULL;
    }

/* This came from MorphOS source code, however looks strange.
   Commented out but left for reference.    
    if (dl)
        Forbid(); */
    if (!dl) {
        if (DevSem)
            ReleaseSemaphore(&di->di_DevLock);
        if (EntrySem)
            ReleaseSemaphore(&di->di_EntryLock);
        if (DelSem)
            ReleaseSemaphore(&di->di_DeleteLock);
    }

    D(bug("AttemptLockDosList: result = $%lx\n", dl));

    return dl;

    AROS_LIBFUNC_EXIT
} /* AttemptLockDosList */
