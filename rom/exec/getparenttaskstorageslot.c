/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include "taskstorage.h"

/*****************************************************************************

    NAME */
#include <proto/exec.h>

        AROS_LH1(IPTR, GetParentTaskStorageSlot,

/*  LOCATION */
        AROS_LHA(LONG, id, D0),
        struct ExecBase *, SysBase, 186, Exec)

/*  FUNCTION
        Get a value for a task storage slot of parent task.

    INPUTS
        id - slot ID returned from AllocTaskStorageSlot().

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

    struct ETask *et = GetETask(FindTask(NULL));
    IPTR result = (IPTR)NULL;

    if (!et)
        return (IPTR)NULL;

    Forbid(); /* Accessing other task's et_TaskStorage */

    result = TaskGetStorageSlot(et->et_Parent, id);

    Permit();

    return result;

    AROS_LIBFUNC_EXIT
}
