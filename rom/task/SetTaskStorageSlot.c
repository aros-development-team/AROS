/*
    Copyright (C) 2012-2023, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include <exec/nodes.h>
#include <exec/lists.h>

#include <string.h>

#include "task_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH2(BOOL, SetTaskStorageSlot,

/*  LOCATION */
        AROS_LHA(LONG, slotid, D0),
        AROS_LHA(IPTR , value, D1),
        struct TaskResBase *, TaskResBase, 16, Task)

/*  FUNCTION
        Puts a new value in a task storage slot. If necessary, the number of
        task storage slotalloccnt will be increased.

    INPUTS
        slotid - slot ID returned from AllocTaskStorageSlot().
        value - value to store in the slot.

    RESULT
        success - TRUE if the value was successfully stored.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        AllocTaskStorageSlot(), FreeTaskStorageSlot(), GetTaskStorageSlot()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#ifdef TASKRES_ENABLE
    struct Task *thisTask = GET_THIS_TASK;
    struct ETask *et = thisTask ? GetETask(thisTask) : NULL;
    IPTR *tsstorage;

    D(bug("[TaskRes] %s: %p: Set TaskStorageSlot %u to 0x%p\n", __func__, et, slotid, (APTR)value);)

    if (!et) {
        /* Only ETasks can do this */
        D(bug("[TaskRes] %s: Missing ETask!\n", __func__);)
        return FALSE;
    }

    /* Valid ID? */
    if (slotid < 1) {
        D(bug("[TaskRes] %s: Invalid SlotID %d\n", __func__, slotid);)
        return FALSE;
    }

    tsstorage = (IPTR *)et->et_Reserved[ETASK_RSVD_SLOTID];
    if (tsstorage == NULL) {
        /* No TaskStorage? Allocate some. */
        ULONG slotalloccnt = ((slotid + TASKSTORAGEPUDDLE) / TASKSTORAGEPUDDLE) * TASKSTORAGEPUDDLE;
        tsstorage = AllocMem(slotalloccnt * sizeof(tsstorage[0]), MEMF_ANY | MEMF_CLEAR);
        if (tsstorage == NULL) {
            D(bug("[TaskRes] %s: Failed to allocate %u slotalloccnt\n", __func__, slotalloccnt);)
            return FALSE;
        }

        tsstorage[__TS_FIRSTSLOT] = slotalloccnt;

        /* Replacing of the pointer needs to be atomic due to GetParentTaskStorageSlot() */
        et->et_Reserved[ETASK_RSVD_SLOTID] = tsstorage;
    } else if (tsstorage[__TS_FIRSTSLOT] <= slotid) {
        IPTR *newts;
        ULONG newslots = ((slotid + TASKSTORAGEPUDDLE) / TASKSTORAGEPUDDLE) * TASKSTORAGEPUDDLE;
        size_t oldslots = tsstorage[__TS_FIRSTSLOT];

        newts = AllocMem(newslots * sizeof(tsstorage[0]), MEMF_ANY);
        if (newts == NULL) {
            D(bug("[TaskRes] %s: Failed to allocate %u new slotalloccnt\n", __func__, newslots);)
            return FALSE;
        }

        newts[__TS_FIRSTSLOT] = newslots;

        CopyMem(&tsstorage[__TS_FIRSTSLOT+1], &newts[__TS_FIRSTSLOT+1],
                (oldslots-1) * sizeof(tsstorage[0]));
        memset(&newts[oldslots], 0, (newslots - oldslots) * sizeof(tsstorage[0]));

        /* Replacing of the pointer needs to be atomic due to GetParentTaskStorageSlot() */
        et->et_Reserved[ETASK_RSVD_SLOTID] = newts;
        FreeMem(tsstorage, oldslots * sizeof(tsstorage[0]));
    }

    tsstorage = (IPTR *)et->et_Reserved[ETASK_RSVD_SLOTID];
    if (tsstorage != NULL)
        tsstorage[slotid] = value;

    return TRUE;
#else
    return FALSE;
#endif

    AROS_LIBFUNC_EXIT
} /* SetTaskStorageSlot() */
