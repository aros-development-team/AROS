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
#include "exec_util.h"
#include "etask.h"
#include "taskstorage.h"

/*****************************************************************************

    NAME */
#include <proto/exec.h>

        AROS_LH1(BOOL, Dispatch,

/*  SYNOPSIS */
        AROS_LHA(struct Task *, task, A0),

/*  LOCATION */
        struct ExecBase *, SysBase, 10, Exec)

/*  FUNCTION
        Perform dispatch-time task maintenance.

    INPUTS
        task - a task to be dispatched.

    RESULT
        TRUE if dispatching permitted, FALSE if dispatching needs to be
        cancelled

    NOTES
        This is a very private function.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    int oldstorage;

    if (task == PrivExecBase(SysBase)->ServicePort->mp_SigTask)
    {
        /*
         * We can't ask housekeeper to service itself.
         * It's its own responsibility to take care about itself.
         */
        return TRUE;
    }

    oldstorage = task->tc_UnionETask.tc_TaskStorage[__TS_FIRSTSLOT];
    if (oldstorage < PrivExecBase(SysBase)->TaskStorageSize)
    {
        D(bug("[Dispatch] Task 0x%p <%s> needs TSS increase (%d -> %d)\n", task, task->tc_Node.ln_Name, oldstorage, PrivExecBase(SysBase)->TaskStorageSize));

        /*
         * The task has been removed from TaskReady list by kernel.
         * Send it to housekeeper for servicing.
         * We use InternalPutMsg() because it won't clobber ln_Type.
         */
        InternalPutMsg(((struct IntExecBase *)SysBase)->ServicePort, (struct Message *)task, SysBase);

        return FALSE;
    }

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* Dispatch() */
