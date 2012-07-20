/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/nodes.h>
#include <exec/lists.h>

#include "exec_intern.h"
#include "taskstorage.h"

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/exec.h>

        AROS_LH0(APTR, SaveTaskStorage,

/*  LOCATION */
        struct ExecBase *, SysBase, 182, Exec)

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
        RestoreTaskStorage()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ETask *et = GetETask(FindTask(NULL));
    IPTR *taskstorage, *tsout;
    IPTR slots;
    
    if (!et || et->et_TaskStorage == NULL)
        return NULL;

    taskstorage = et->et_TaskStorage;
    slots = taskstorage[__TS_FIRSTSLOT];

    /* NOTE: Saved task storage *must not* be passed around
     *       to another task! This is why it is not MEMF_PUBLIC.
     */
    tsout = AllocMem(slots * sizeof(tsout[0]), MEMF_ANY);
    if (tsout) {
        CopyMemQuick(taskstorage, tsout, slots * sizeof(tsout[0]));
    }

    D(bug("SaveTaskStorage: taskstorage=%x, size=%d, tsout=%x\n",
          taskstorage, size, tsout
    ));

    return tsout;

    AROS_LIBFUNC_EXIT
}
