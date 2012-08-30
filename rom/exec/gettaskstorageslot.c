/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
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
        Value stored by SetTaskStorage(), or (IPTR)NULL if the slot was
        never used.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        AllocTaskStorage(), FreeTaskStorage(), SetTaskStorageSlot()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ETask *et = GetETask(FindTask(NULL));
    IPTR *ts;

    D(bug("GetTaskStorage: %p: Get TaskStorageSlot %d\n", et, id));

    if (!et) {
        /* Only ETasks can do this */
        D(bug("GetTaskStorage: Not an ETask!\n"));
        return (IPTR)NULL;
    }

    ts = et->et_TaskStorage;
    if (ts == NULL || ts[__TS_FIRSTSLOT] <= id) {
        D(bug("GetTaskStorage: ID %d was not set!\n", id));
        return (IPTR)NULL;
    }

    return ts[id];

    AROS_LIBFUNC_EXIT
}
