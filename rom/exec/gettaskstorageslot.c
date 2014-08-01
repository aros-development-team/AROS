/*
    Copyright © 2012-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <exec/nodes.h>
#include <exec/lists.h>
#include <clib/macros.h>

#include "exec_intern.h"
#include "taskstorage.h"

/*****************************************************************************

    NAME */
#include <proto/exec.h>

        AROS_LH1(IPTR, GetTaskStorageSlot,

/*  LOCATION */
        AROS_LHA(LONG, id, D0),
        struct ExecBase *, SysBase, 185, Exec)

/*  FUNCTION
        Get a value for a task storage slot.

    INPUTS
        id - slot ID returned from AllocTaskStorageSlot().

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

    return TaskGetStorageSlot(FindTask(NULL), id);

    AROS_LIBFUNC_EXIT
}

IPTR TaskGetStorageSlot(struct Task * t, LONG id)
{
    struct ETask *et = GetETask(t);
    IPTR *ts;

    D(bug("TaskGetStorageSlot: %p: Get TaskGetStorageSlot %d\n", et, id));

    if (!et) {
        /* Only ETasks can do this */
        D(bug("TaskGetStorageSlot: Not an ETask!\n"));
        return (IPTR)NULL;
    }

    ts = et->et_TaskStorage;
    if (ts == NULL || ts[__TS_FIRSTSLOT] <= id) {
        D(bug("TaskGetStorageSlot: ID %d was not set!\n", id));
        return (IPTR)NULL;
    }

    return ts[id];
}
