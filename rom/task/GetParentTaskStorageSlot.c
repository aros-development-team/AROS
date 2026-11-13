/*
    Copyright (C) 2014-2023, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include "task_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH1(IPTR, GetParentTaskStorageSlot,

/*  LOCATION */
        AROS_LHA(LONG, slotid, D0),
        struct TaskResBase *, TaskResBase, 18, Task)

/*  FUNCTION
        Get a value for a task storage slot of parent task.

    INPUTS
        slotid - slot ID returned from AllocTaskStorageSlot().

    RESULT
        Value stored by SetTaskStorageSlot() on parent task, or
        (IPTR)NULL if the slot was never used.

    NOTES
        Since you are accessing value of another task, the value
        might be invalid/freed by the time this function returns.
        To be sure value is still valid, call this function under
        Forbid().

    EXAMPLE

    BUGS

    SEE ALSO
        AllocTaskStorageSlot(), FreeTaskStorageSlot(), SetTaskStorageSlot(),
        GetTaskStorageSlot()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    IPTR result = (IPTR)NULL;

#ifdef TASKRES_ENABLE
    struct Task *thisTask = GET_THIS_TASK;
    struct ETask *et = thisTask ? GetETask(thisTask) : NULL;

    if (!et)
        return (IPTR)NULL;

    Forbid(); /* Accessing other task's et_Reserved[ETASK_RSVD_SLOTID] */

    if (et->et_Parent)
        result = TaskGetStorageSlot(et->et_Parent, slotid);

    Permit();
#endif

    D(bug("[TaskRes] %s: Returning $%p <Parent SlotID #%u>\n", __func__, result, slotid);)

    return result;

    AROS_LIBFUNC_EXIT
} /* GetParentTaskStorageSlot() */
