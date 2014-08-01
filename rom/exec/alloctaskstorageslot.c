/*
    Copyright � 2011-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "taskstorage.h"

/*****************************************************************************

    NAME */
#include <proto/exec.h>

        AROS_LH0(LONG, AllocTaskStorageSlot,

/*  LOCATION */
        struct ExecBase *, SysBase, 180, Exec)

/*  FUNCTION
        This function will allocate a slot in the taskstorage.

    INPUTS
        None.

    RESULT
        slot - The allocated slot, or 0 if no slot could be allocated.

    NOTES
        After this function SetTaskStorageSlot(slot) may be used to store
        values with each slot.

    EXAMPLE

    BUGS

    SEE ALSO
        FreeTaskStorageSlot(), GetTaskStorageSlot(), SetTaskStorageSlot()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TaskStorageFreeSlot *tsfs;
    LONG slot;
    struct IntETask *iet = GetIntETask(FindTask(NULL));

    if (!iet)
        return 0;

    Forbid();
    tsfs = (struct TaskStorageFreeSlot *)
        GetHead(&PrivExecBase(SysBase)->TaskStorageSlots);
    if (!tsfs)
        Alert(AT_DeadEnd|AN_MemoryInsane);

    slot = tsfs->FreeSlot;

    D(bug("[TSS] Task 0x%p (%s): Allocated slot %d\n", FindTask(NULL), FindTask(NULL)->tc_Node.ln_Name, slot));

    if (GetSucc(tsfs) == NULL)
    {
        /* Last element always points to highest element and is a new slot */
        tsfs->FreeSlot++;
    }
    else
    {
        Remove((struct Node *) tsfs);
        FreeMem(tsfs, sizeof(struct TaskStorageFreeSlot));
    }
    Permit();

    return slot;

    AROS_LIBFUNC_EXIT
}
