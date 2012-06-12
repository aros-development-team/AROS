/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove a task
    Lang: english
*/

#include <aros/debug.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <aros/symbolsets.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "exec_debug.h"

/*****************************************************************************

    NAME */

        AROS_LH1(void, RemTask,

/*  SYNOPSIS */
        AROS_LHA(struct Task *, task, A1),

/*  LOCATION */
        struct ExecBase *, SysBase, 48, Exec)

/*  FUNCTION
        Remove a task from the task lists. All memory in the tc_MemEntry list
        is freed and a reschedule is done. It's safe to call RemTask() outside
        Forbid() or Disable().

        This function is one way to get rid of the current task. The other way
        is to fall through the end of the entry point.

    INPUTS
        task - Task to be removed. NULL means current task.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        AddTask()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct MemList *mb;
    struct ETask *et;
    BOOL suicide;

    /* A value of NULL means current task */
    if (task==NULL)
        task=SysBase->ThisTask;

    DREMTASK("RemTask (0x%p (\"%s\"))", task, task->tc_Node.ln_Name);

    /* Don't let any other task interfere with us at the moment
    */
    Forbid();

    suicide = (task == SysBase->ThisTask);
    if (suicide)
        DREMTASK("Removing itself");

    /* Remove() here, before freeing the MemEntry list. Because
       the MemEntry list might contain the task struct itself! */

    if (!suicide)
        Remove(&task->tc_Node);

    /*
     * The task is being removed.
     * This is an important signal for Alert() which will not attempt to use
     * the context which is being deleted, for example.
     */
    task->tc_State = TS_REMOVED;

    /* Delete context */
    et = GetETask(task);
    if (et != NULL)
        KrnDeleteContext(et->et_RegFrame);

    /* Uninitialize ETask structure */
    DREMTASK("Cleaning up ETask et=%p", et);
    CleanupETask(task);

    /* Freeing myself? */
    if (suicide)
    {
        /*
         * Send task to task cleaner to clean up memory. This avoids ripping
         * memory from underneath a running Task. Message is basically a
         * Node, so we use our task's tc_Node as a message. We use
         * InternalPutMsg() because it won't change ln_Type. Just in case...
         */
        DREMTASK("Sending to garbage man");
        InternalPutMsg(((struct IntExecBase *)SysBase)->ServicePort,
            (struct Message *)task, SysBase);

        /* Changing the task lists always needs a Disable(). */
        Disable();

        /*
            Since I don't know how many levels of Forbid()
            are already pending I set a default value.
        */
        SysBase->TDNestCnt = -1;

        /* And force a task switch. Note: Dispatch, not Switch,
           because the state of ThisTask must not be saved
        */

        KrnDispatch();
        /* Does not return. */
    }
    else
    {
        /*
         * Free all memory in the tc_MemEntry list.
         * We do this here because it's unsafe to send it to the service task:
         * by the time the service task processes it, the task structure may
         * have been freed following the return of this function.
         */
        while((mb = (struct MemList *) RemHead(&task->tc_MemEntry)) != NULL)
            FreeEntry(mb);
    }

    /* All done. */
    Permit();

    DREMTASK("Success");

    AROS_LIBFUNC_EXIT
}
