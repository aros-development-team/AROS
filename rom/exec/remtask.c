/*
    Copyright  1995-2011, The AROS Development Team. All rights reserved.
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

#include "etask.h"
#include "exec_intern.h"
#include "exec_util.h"
#include "exec_debug.h"

/*****************************************************************************

    NAME */

        AROS_LH1(void, RemTask,

/*  SYNOPSIS */
        AROS_LHA(struct Task *,     task, A1),

/*  LOCATION */
        struct ExecBase *, SysBase, 48, Exec)

/*  FUNCTION
        Remove a task from the task lists. All memory in the tc_MemEntry list
        is freed and a rescedule is done. It's safe to call RemTask() out
        of Forbid() or Disable().
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

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct ETask *et;

    /* A value of NULL means current task */
    if (task==NULL)
        task=SysBase->ThisTask;

    DREMTASK("RemTask (0x%p (\"%s\"))", task, task->tc_Node.ln_Name);

    if (task == SysBase->ThisTask)
    	DREMTASK("Removing itself");

    /* Don't let any other task interfere with us at the moment
    */
    Forbid();

    /* Remove() here, before freeing the MemEntry list. Because
       the MemEntry list might contain the task struct itself! */

    if(task != SysBase->ThisTask)
    {
        Remove(&task->tc_Node);
    }

    /*
     * The task is being removed.
     * This is an important signal for Alert() which will not attempt to use
     * the context which is being deleted, for example.
     */
    task->tc_State = TS_REMOVED;

    /* Delete context */
    et = GetETask(task);
    if(et != NULL)
        KrnDeleteContext(((struct IntETask *)et)->iet_Context);

    /* Uninitialize ETask structure */
    CleanupETask(task);

    /*
     * Send task to task cleaner to clean up memory.
     * This avoids ripping memory from underneath a running Task.
     * Message is basically a Node, so we use our task's tc_Node as a message.
     * We use InternalPutMsg() because it won't change ln_Type. Just in case...
     */
    InternalPutMsg(((struct IntExecBase *)SysBase)->RemTaskPort, (struct Message *)task, SysBase);

    /* Freeing myself? */
    if(task==SysBase->ThisTask)
    {
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

    /* All done. */
    Permit();

    DREMTASK("Success");

    AROS_LIBFUNC_EXIT
}

static void remtaskcleaner(void)
{
    struct MemList *mb, *mbnext;
    struct IntExecBase *IntSysBase = (struct IntExecBase *)SysBase;

    DREMTASK("remtaskcleaner RemTaskPort created");

    do
    { /* forever */
        struct List list;
        struct Task *task;

        WaitPort(IntSysBase->RemTaskPort);
	task = (struct Task *)GetMsg(IntSysBase->RemTaskPort);

        DREMTASK("remtaskcleaner for task %p", task);

        /* Note tc_MemEntry list is part of the task structure which
           usually is also placed in tc_MemEntry. MungWall_Check()
           will fill freed memory and destroy our list while we are
           iterating or the freed memory including our list could be
           reused by some other task. We take special care of this by
           copying the list nodes to a local list before freeing.
           Alternatively, we could check all MemEntries for the task
           and free it after iterating.
         */
        NEWLIST(&list);
        ForeachNodeSafe(&task->tc_MemEntry, mb, mbnext)
        {
            Remove(&mb->ml_Node);
            AddTail(&list, &mb->ml_Node);
        }
        ForeachNodeSafe(&list, mb, mbnext)
        {
            DREMTASK("remtaskcleaner freeing MemList 0x%p", mb);
            /* Free one MemList node */
            FreeEntry(mb);
        }
    } while(1);
}

int __RemTask_Setup(struct ExecBase *SysBase)
{
    struct Task *cleaner;

    /* taskpri is 127, we assume this task will be run before another task
       calls RemTask()
    */
    cleaner = NewCreateTask(TASKTAG_NAME       , "__RemTask_Cleaner__",
    		            TASKTAG_PRI	       , 127,
    		            TASKTAG_PC	       , remtaskcleaner,
    		            TASKTAG_TASKMSGPORT, &((struct IntExecBase *)SysBase)->RemTaskPort,
    		            TAG_DONE);

    if (!cleaner)
    {
        DREMTASK("__RemTask_Setup task creation failed !");
        return 0;
    }
    DREMTASK("__RemTask_Setup cleaner task created");

    return 1;
}

ADD2INITLIB(__RemTask_Setup, 0);
