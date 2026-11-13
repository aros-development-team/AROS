/*
    Copyright (C) 2011-2023, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include "task_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH0(LONG, AllocTaskStorageSlot,

/*  LOCATION */
        struct TaskResBase *, TaskResBase, 12, Task)

/*  FUNCTION
        This function will allocate a slot in the task storage.

    INPUTS
        None.

    RESULT
        The allocated SlotID, or 0 if no slotid could be allocated.

    NOTES
        After this function SetTaskStorageSlot(slotid) may be used to store
        values with each slotid.

    EXAMPLE

    BUGS

    SEE ALSO
        FreeTaskStorageSlot(), GetTaskStorageSlot(), SetTaskStorageSlot()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG slotid = 0;

#ifdef TASKRES_ENABLE
    struct Task *thisTask = GET_THIS_TASK;
    struct TaskStorageFreeSlot *freeTaskStorageSlot;
    struct IntETask *iet = thisTask ? GetIntETask(thisTask) : NULL;

    if (iet)
    {
        Forbid();
        freeTaskStorageSlot = (struct TaskStorageFreeSlot *)
            GetHead(&TaskResBase->trb_TaskStorageSlots);
        if (!freeTaskStorageSlot)
        {
            Alert(AT_DeadEnd|AN_MemoryInsane);
            __builtin_unreachable();
        }

        slotid = freeTaskStorageSlot->FreeSlot;

        D(bug("[TaskRes] %s: Task '%s' Allocated SlotID #%u\n", __func__, thisTask->tc_Node.ln_Name, slotid);)

        if (GetSucc(freeTaskStorageSlot) == NULL)
        {
            /* Last element always points to highest element and is a new slotid */
            freeTaskStorageSlot->FreeSlot++;
        }
        else
        {
            Remove((struct Node *) freeTaskStorageSlot);
            FreeMem(freeTaskStorageSlot, sizeof(struct TaskStorageFreeSlot));
        }
        Permit();
    }
#endif

    return slotid;

    AROS_LIBFUNC_EXIT
} /* AllocTaskStorageSlot() */
