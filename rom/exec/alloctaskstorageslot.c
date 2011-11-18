/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "taskstorage.h"

/*****************************************************************************

    NAME */
#include <proto/exec.h>

        AROS_LH0(int, AllocTaskStorageSlot,

/*  LOCATION */
        struct ExecBase *, SysBase, 139, Exec)

/*  FUNCTION
        This function will allocate a slot in the taskstorage 

    INPUTS
        None.

    RESULT
        The allocated slot, 0 if no slot could be allocated.

    NOTES
        After this function FindTask(NULL)->tc_UnionETask.tc_TaskStorage[slot]
        may be used to store values with each slot. Data stored in a slot
        will be duplicated when a Task creates another Task. It is left up to the
        to implement a mechanism to check if this copied value is invalid.

    EXAMPLE

    BUGS

    SEE ALSO
        FreeTaskStorageSlot()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TaskStorageFreeSlot *tsfs = (struct TaskStorageFreeSlot *)GetHead(&PrivExecBase(SysBase)->TaskStorageSlots);
    int slot;
    struct Task *t;

    if (!tsfs)
        Alert(AT_DeadEnd|AN_MemoryInsane);

    slot = tsfs->FreeSlot;

    D(bug("[TSS] Task 0x%p (%s): Allocated slot %d\n", FindTask(NULL), FindTask(NULL)->tc_Node.ln_Name, slot));

    if ((slot + 1)*sizeof(IPTR) > PrivExecBase(SysBase)->TaskStorageSize)
    {
        t = FindTask(NULL);

        if (!Exec_ExpandTS(t, SysBase))
            return 0;
    }

    if (GetSucc(tsfs) == NULL)
    {
        /* Last element always points to highest element and is a new slot */
        tsfs->FreeSlot++;
    }
    else
    {
        Remove((struct Node *)tsfs);
        FreeMem(tsfs, sizeof(struct TaskStorageFreeSlot));

        /* Clean previous values in slot on all tasks */
        Forbid();
        ForeachNode(&SysBase->TaskReady, t)
        {
            if (t->tc_Flags & TF_ETASK)
                t->tc_UnionETask.tc_TaskStorage[slot] = (IPTR)0;
        }
        ForeachNode(&SysBase->TaskWait, t)
        {
            if (t->tc_Flags & TF_ETASK)
                t->tc_UnionETask.tc_TaskStorage[slot] = (IPTR)0;
        }
        Permit();
    }

    return slot;

    AROS_LIBFUNC_EXIT
}
