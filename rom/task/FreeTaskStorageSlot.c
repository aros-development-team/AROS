/*
    Copyright (C) 2011-2023, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#include "task_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH1(void, FreeTaskStorageSlot,

/*  LOCATION */
        AROS_LHA(LONG, slotid, D0),
        struct TaskResBase *, TaskResBase, 13, Task)

/*  FUNCTION
        This function will free a slot in task storage

    INPUTS
        slotid - The slot id to free.

    RESULT
        None.

    NOTES
        Currently no checks are performed to determine if one is the owner
        of the slotid. This may be added in the future, so one should
        deallocate a slotid from the same task that allocated the slotid.

    EXAMPLE

    BUGS

    SEE ALSO
        AllocTaskStorageSlot()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#ifdef TASKRES_ENABLE
    struct TaskStorageFreeSlot *freeTaskStorageSlot =
        AllocMem(sizeof(struct TaskStorageFreeSlot), MEMF_PUBLIC|MEMF_CLEAR);

    if (freeTaskStorageSlot == NULL)
    {
        /* Don't do anything, we'll just lose the freed slotid */
        return;
    }

    freeTaskStorageSlot->FreeSlot = slotid;
    D(
        struct Task *thisTask = GET_THIS_TASK;
        bug("[TaskRes] %s: Task '%s' Freed SlotID #%u\n", __func__, thisTask->tc_Node.ln_Name, slotid);
    )

    Forbid();
    AddHead((struct List *)&TaskResBase->trb_TaskStorageSlots, (struct Node *)freeTaskStorageSlot);
    Permit();
#endif

    AROS_LIBFUNC_EXIT
} /* FreeTaskStorageSlot() */
