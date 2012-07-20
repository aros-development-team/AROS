/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <exec/nodes.h>
#include <exec/lists.h>

#include "exec_intern.h"
#include "taskstorage.h"

/*****************************************************************************

    NAME */
#include <proto/exec.h>

        AROS_LH2(BOOL, SetTaskStorageSlot,

/*  LOCATION */
        AROS_LHA(LONG, id, D0),
        AROS_LHA(IPTR , value, D1),
        struct ExecBase *, SysBase, 184, Exec)

/*  FUNCTION
        The will remember the current state of the task storage slots

    INPUTS
        None.

    RESULT
        An id will be returned whit which the current state can be restored
        using RestoreTaskStorageSlots().
        NULL is returned when not enough memory was available.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        AllocTaskStorageSlot()/FreeTaskStorageSlot()/GetTaskStorageSlot()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ETask *et = GetETask(FindTask(NULL));
    IPTR *ts;

    D(bug("SetTaskStorage: %p: Set TaskStorageSlot %d to %p\n", et, id, (APTR)value));

    if (!et) {
        /* Only ETasks can do this */
        D(bug("SetTaskStorage: Not an ETask!\n"));
        return FALSE;
    }

    /* Valid ID? */
    if (id < 1) {
        D(bug("SetTaskStorage: Invalid ID %d\n", id));
        return FALSE;
    }

    ts = et->et_TaskStorage;
    if (ts == NULL) {
        /* No TaskStorage? Allocate some. */
        ULONG slots = ((id + TASKSTORAGEPUDDLE) / TASKSTORAGEPUDDLE) * TASKSTORAGEPUDDLE;
        ts = AllocMem(slots * sizeof(ts[0]), MEMF_ANY | MEMF_CLEAR);
        if (ts == NULL) {
            D(bug("SetTaskStorage: Can't allocate %d slots\n", slots));
            return FALSE;
        }

        ts[__TS_FIRSTSLOT] = slots;

        et->et_TaskStorage = ts;
    } else if (ts[__TS_FIRSTSLOT] <= id) {
        IPTR *newts;
        ULONG newslots = ((id + TASKSTORAGEPUDDLE) / TASKSTORAGEPUDDLE) * TASKSTORAGEPUDDLE;
        size_t oldslots = ts[__TS_FIRSTSLOT];

        newts = AllocMem(newslots * sizeof(ts[0]), MEMF_ANY);
        if (newts == NULL) {
            D(bug("SetTaskStorage: Can't allocate %d new slots\n", newslots));
            return FALSE;
        }

        newts[__TS_FIRSTSLOT] = newslots;

        CopyMem(&ts[__TS_FIRSTSLOT+1], &newts[__TS_FIRSTSLOT+1],
                (oldslots-1) * sizeof(ts[0]));
        memset(&newts[oldslots], 0, (newslots - oldslots) * sizeof(ts[0]));

        et->et_TaskStorage = newts;
        FreeMem(ts, oldslots * sizeof(ts[0]));
    }

    et->et_TaskStorage[id] = value;

    return TRUE;

    AROS_LIBFUNC_EXIT
}
