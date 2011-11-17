/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Dispatch() - Exec-side task dispatch handling
    Lang: english
*/

#include <aros/debug.h>
#include <exec/lists.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "etask.h"
#include "taskstorage.h"

/*****************************************************************************

    NAME */
#include <proto/exec.h>

        AROS_LH1(struct Task *, Dispatch,

/*  SYNOPSIS */
        AROS_LHA(struct Task *, task, A0),

/*  LOCATION */
        struct ExecBase *, SysBase, 10, Exec)

/*  FUNCTION
        Perform dispatch-time task maintenance.

    INPUTS
        task - a task to be dispatched.

    RESULT

    NOTES
        This is a very private function.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /*
     * Increase TaskStorage if it is not big enough.
     * Please don't even look at this crap. All this will be rewritten.
     */
    IPTR *oldstorage = task->tc_UnionETask.tc_TaskStorage;

    if ((int)oldstorage[__TS_FIRSTSLOT] < PrivExecBase(SysBase)->TaskStorageSize)
    {
        IPTR *newstorage;
        ULONG oldsize = (ULONG)oldstorage[__TS_FIRSTSLOT];

	D(bug("[Dispatch] Increasing storage (%d to %d) for task 0x%p (%s)\n", oldsize, PrivExecBase(SysBase)->TaskStorageSize, task, task->tc_Node.ln_Name));

        newstorage = AllocMem(PrivExecBase(SysBase)->TaskStorageSize, MEMF_PUBLIC|MEMF_CLEAR);
        /* FIXME: Add fault handling */

        CopyMem(oldstorage, newstorage, oldsize);
        newstorage[__TS_FIRSTSLOT] = PrivExecBase(SysBase)->TaskStorageSize;
        task->tc_UnionETask.tc_TaskStorage = newstorage;
        FreeMem(oldstorage, oldsize);
    }

    return task;

    AROS_LIBFUNC_EXIT
} /* Dispatch() */
