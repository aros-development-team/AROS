/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "exec_intern.h"
#include "taskstorage.h"

/*****************************************************************************

    NAME */
#include <proto/exec.h>

        AROS_LH1(void, FreeTaskStorageSlot,

/*  LOCATION */
        AROS_LHA(LONG, slot, D0),
        struct ExecBase *, SysBase, 181, Exec)

/*  FUNCTION
        This function will free a slot in taskstorage 

    INPUTS
	slot - The slot to free.

    RESULT
        None.

    NOTES
        Currently no checks are performed to determine if one is the owner
        of the slot. This may be added in the future, so one should
        deallocate a slot from the same task that allocated the slot.

    EXAMPLE

    BUGS

    SEE ALSO
	AllocTaskStorageSlot()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TaskStorageFreeSlot *tsfs =
        AllocMem(sizeof(struct TaskStorageFreeSlot), MEMF_PUBLIC|MEMF_CLEAR);

    if (tsfs == NULL)
    {
        /* Don't do anything, we'll just lose the freed slot */
        return;
    }

    tsfs->FreeSlot = slot;

    Forbid();
    AddHead((struct List *)&PrivExecBase(SysBase)->TaskStorageSlots, (struct Node *)tsfs);
    Permit();

    AROS_LIBFUNC_EXIT
}
