/*
    Copyright (C) 2012-2023, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include <exec/nodes.h>
#include <exec/lists.h>
#include <clib/macros.h>

#include "task_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH1(IPTR, GetTaskStorageSlot,

/*  LOCATION */
        AROS_LHA(LONG, slotid, D0),
        struct TaskResBase *, TaskResBase, 17, Task)

/*  FUNCTION
        Get a value for a task storage slot.

    INPUTS
        slotid - slot ID returned from AllocTaskStorageSlot().

    RESULT
        Value stored by SetTaskStorageSlot(), or (IPTR)NULL if the slot was
        never used.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        AllocTaskStorageSlot(), FreeTaskStorageSlot(), SetTaskStorageSlot()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#ifdef TASKRES_ENABLE
    return TaskGetStorageSlot(GET_THIS_TASK, slotid);
#else
    return NULL;
#endif

    AROS_LIBFUNC_EXIT
} /* GetTaskStorageSlot() */

#ifdef TASKRES_ENABLE
IPTR TaskGetStorageSlot(struct Task * t, LONG slotid)
{
    struct ETask *et = t ? GetETask(t) : NULL;
    IPTR *tsstorage;

    D(bug("[TaskRes] %s: ETask @ 0x%p SlotID #%u\n", __func__, et, slotid);)

    if (!et) {
        /* Only ETasks can do this */
        D(bug("[TaskRes] %s: Missing ETask!\n", __func__);)
        return (IPTR)NULL;
    }

    tsstorage = et->et_Reserved[ETASK_RSVD_SLOTID];
    if (tsstorage == NULL || tsstorage[__TS_FIRSTSLOT] <= slotid) {
        D(bug("[TaskRes] %s: SlotID #%u was not set!\n", __func__, slotid);)
        return (IPTR)NULL;
    }

    return tsstorage[slotid];
}
#endif
