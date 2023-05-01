/*
    Copyright (C) 2012-2023, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include <exec/nodes.h>
#include <exec/lists.h>

#include "task_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH0(APTR, SaveTaskStorage,

/*  LOCATION */
        struct TaskResBase *, TaskResBase, 14, Task)

/*  FUNCTION
        This function remembers the current state of the task storage slots.
        An ID will be returned with which the current state can be restored
        using RestoreTaskStorage(). NULL is returned when not enough memory
        is available.

    INPUTS
        None.

    RESULT
        id - ID for use with RestoreTaskStorage(), or NULL.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        RestoreTaskStorage()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    IPTR *tsout = NULL;
#ifdef TASKRES_ENABLE
    struct Task *thisTask = GET_THIS_TASK;
    struct ETask *et = thisTask ? GetETask(thisTask) : NULL;
    IPTR *tsstorage;
    IPTR slots;
    
    if (et && et->et_Reserved[ETASK_RSVD_SLOTID] != NULL)
    {
        tsstorage = et->et_Reserved[ETASK_RSVD_SLOTID];
        slots = tsstorage[__TS_FIRSTSLOT];

        /* NOTE: Saved task storage *must not* be passed around
         *       to another task! This is why it is MEMF_PRIVATE.
         */
        tsout = AllocMem(slots * sizeof(tsout[0]), MEMF_PRIVATE);
        if (tsout) {
            CopyMemQuick(tsstorage, tsout, slots * sizeof(tsout[0]));
        }

        D(bug("[TaskRes] %s: Task Slot Storage @ 0x%p, size=%d <Saved @ 0x%p>\n",
              __func__, tsstorage, size, tsout
        );)
    }
#endif

    return tsout;

    AROS_LIBFUNC_EXIT
} /* SaveTaskStorage() */
