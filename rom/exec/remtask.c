/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove a task
    Lang: english
*/

#include <exec/execbase.h>
#include <exec/tasks.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

#include "etask.h"
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
    struct Task *suicide = GET_THIS_TASK;

    /* A value of NULL means current task */
    if (task == NULL)
        task = suicide;

    DREMTASK("RemTask (0x%p (\"%s\"))", task, task->tc_Node.ln_Name);

    /* Don't let any other task interfere with us at the moment */
    Forbid();

    /* Dont try to kill a dying task .. */
    if ((task->tc_State != TS_INVALID) &&
        (task->tc_State != TS_TOMBSTONED) &&
        (task->tc_State != TS_REMOVED))
    {
        /*
         * The task is being removed.
         * This is an important signal for Alert() which will not attempt to use
         * the context which is being deleted, for example.
         */

        if (suicide == task)
        {
            DREMTASK("Removing itself");
            task->tc_State = TS_REMOVED;
#if defined(EXEC_REMTASK_NEEDSSWITCH)
            // make the scheduler detach us...
            krnSysCallSwitch();
#endif
        }
        else
        {
            /*
             * Remove() here, before freeing the MemEntry list. Because
             * the MemEntry list might contain the task struct itself!
            */
#if !defined(EXEC_REMTASK_NEEDSSWITCH)
            task->tc_State = TS_REMOVED;
            Remove(&task->tc_Node);
#else
            krnSysCallReschedTask(task, TS_REMOVED);
#endif
        }

        /* Delete context */
        et = GetETask(task);
        if (et != NULL)
        {
#if defined(__AROSEXEC_SMP__)
            // We must cache the CPU number here for the Exec Service Task...
            task->tc_UserData = (APTR)IntETask(et)->iet_CpuNumber;
#endif
            KrnDeleteContext(et->et_RegFrame);
        }

        /* Uninitialize ETask structure */
        DREMTASK("Cleaning up ETask et=%p", et);
        CleanupETask(task);

        /* Freeing itself? */
        if (suicide == task)
        {
            /*
             * Send task to task cleaner to clean up memory. This avoids ripping
             * memory from underneath a running Task. Message is basically a
             * Node, so we use our task's tc_Node as a message. We use
             * InternalPutMsg() because it won't change ln_Type. Just in case...
             */
            DREMTASK("Sending to Exec service task");
            InternalPutMsg(PrivExecBase(SysBase)->ServicePort,
                (struct Message *)task, SysBase);

            DREMTASK("Disabling interrupts");
            /* Changing the task lists always needs a Disable(). */
            Disable();

            DREMTASK("And multitasking..");
            /*
             * We don't know how many levels of Forbid()
             * are already pending, so use a default value.
            */
            TDNESTCOUNT_SET(-1);

            /*
             * Force rescheduling.
             * Note #1: We dont want to preseve the task context so use Dispatch, not Switch.
             * Note #2: We will never return from the dispatch to "ThisTask"
            */

            DREMTASK("Switching away..");
            /* It is  no longer a valid task.. */
            task->tc_State = TS_INVALID;
            KrnDispatch();
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
    }

    /* All done. */
    Permit();

    DREMTASK("Success");

    AROS_LIBFUNC_EXIT
}
