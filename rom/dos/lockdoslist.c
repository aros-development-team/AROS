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

    D(bug("LockDosList: flags = $%lx\n", flags));

    if (((flags & (LDF_READ|LDF_WRITE)) != LDF_READ &&
         (flags & (LDF_READ|LDF_WRITE)) != LDF_WRITE) ||
        (flags & ~(LDF_READ|LDF_WRITE|LDF_DEVICES|LDF_VOLUMES|LDF_ASSIGNS|LDF_ENTRY|LDF_DELETE)))
        return NULL;

    if (flags & LDF_ALL)
    {
        if (flags & LDF_WRITE)
            ObtainSemaphore(&di->di_DevLock);
        else
            ObtainSemaphoreShared(&di->di_DevLock);
    }

    if (flags & LDF_ENTRY)
    {
        if (flags & LDF_WRITE)
            ObtainSemaphore(&di->di_EntryLock);
        else
            ObtainSemaphoreShared(&di->di_EntryLock);
    }

    if (flags & LDF_DELETE)
    {
        if (flags & LDF_WRITE)
            ObtainSemaphore(&di->di_DeleteLock);
        else
            ObtainSemaphoreShared(&di->di_DeleteLock);
    }

/* This strange thing came from MorphOS.
    Forbid(); */

    return (struct DosList *)&di->di_DevInfo;

    AROS_LIBFUNC_EXIT
} /* LockDosList */
