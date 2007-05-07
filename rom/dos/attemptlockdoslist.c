/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
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

    if(flags & LDF_WRITE)
    {
	if(!AttemptSemaphore(&DOSBase->dl_DosListLock))
	    return NULL;
    }
    else
    {
	if(!AttemptSemaphoreShared(&DOSBase->dl_DosListLock))
	    return NULL;
    }

    return (struct DosList *)&DOSBase->dl_DevInfo;

    AROS_LIBFUNC_EXIT
} /* AttemptLockDosList */
