/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/nodes.h>
#include <exec/lists.h>
#include <clib/macros.h>

#include "exec_intern.h"
#include "taskstorage.h"

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/exec.h>

        AROS_LH1(void, RestoreTaskStorage,

/*  LOCATION */
        AROS_LHA(APTR, id, A0),
        struct ExecBase *, SysBase, 183, Exec)

/*  FUNCTION
        This function restores the current state of the task storage slots.

    INPUTS
        id - ID returned from SaveTaskStorage() referring to the state.

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

    struct ETask *et = GetETask(FindTask(NULL));
    IPTR *tsrestore = id;

    if (!et)
        return;

    /* Be sure no other tasks access the information when it is in an
       inconsistent state
    */
    Forbid();

    if (et->et_TaskStorage) {
        IPTR slots = et->et_TaskStorage[__TS_FIRSTSLOT];
        FreeMem(et->et_TaskStorage, slots * sizeof(IPTR));
    }

    /* Restore content of TaskStorage */
    et->et_TaskStorage = tsrestore;

    Permit();

    AROS_LIBFUNC_EXIT
}
