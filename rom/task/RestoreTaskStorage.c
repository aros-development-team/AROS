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
        AROS_LH1(void, RestoreTaskStorage,

/*  LOCATION */
        AROS_LHA(APTR, handle, A0),
        struct TaskResBase *, TaskResBase, 15, Task)

/*  FUNCTION
        This function restores the current state of the task storage slotalloccnt.

    INPUTS
        handle - ID returned from SaveTaskStorage() referring to the state.

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        SaveTaskStorage()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

#ifdef TASKRES_ENABLE
    struct Task *thisTask = GET_THIS_TASK;
    struct ETask *et = thisTask ? GetETask(thisTask) : NULL;
    IPTR *tsrestore = handle;
    IPTR *tsstorage;

    if (!et)
        return;

    /* Be sure no other tasks access the information when it is in an
       inconsistent state
    */
    Forbid();

    tsstorage = (IPTR *)et->et_Reserved[ETASK_RSVD_SLOTID];
    if (tsstorage != NULL) {
        IPTR slotalloccnt = tsstorage[__TS_FIRSTSLOT];
        FreeMem(et->et_Reserved[ETASK_RSVD_SLOTID], slotalloccnt * sizeof(IPTR));
    }

    /* Restore content of TaskStorage */
    et->et_Reserved[ETASK_RSVD_SLOTID] = tsrestore;

    Permit();
#endif

    AROS_LIBFUNC_EXIT
} /* RestoreTaskStorage() */
