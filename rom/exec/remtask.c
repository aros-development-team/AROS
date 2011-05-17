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
    struct MemList *mb, *mbnext;
    struct ETask *et;

    /* A value of NULL means current task */
    if (task==NULL)
        task=SysBase->ThisTask;

    DREMTASK("RemTask (0x%p (\"%s\"))", task, task->tc_Node.ln_Name);

    if (task == SysBase->ThisTask)
    	DREMTASK("Removing itself");

    /*
        Since it's possible that the following will free a task
        structure that is used for some time afterwards it's
        necessary to protect the free memory list so that nobody
        can allocate that memory.
    */
    Forbid();

    /* Remove() here, before freeing the MemEntry list. Because
       the MemEntry list might contain the task struct itself! */
                    
    if(task != SysBase->ThisTask)
    {
        Remove(&task->tc_Node);         
    }

    /* Uninitialize ETask structure */
    et = GetETask(task);
    if(et != NULL)
    {
        KrnDeleteContext(((struct IntETask *)et)->iet_Context);
	CleanupETask(task, et);
    }

    /*
     * The task has been removed.
     * We intentionally set this before FreeEntry() in order to give a warning
     * to mungwall which will not fill freed memory with pattern if it's our
     * current task. See TODO below.
     */
    task->tc_State = TS_REMOVED;

    /*
     * Free all memory in the tc_MemEntry list.
     * TODO: it's a common practice to put struct Task and stack into this list.
     * For example this is done by libamiga's CreateTask() and even by dos.library.
     * We need some smarter way to deallocate these. Current way relies on the fact
     * that the memory still can be physically accessed after being freed. This
     * is not going to be true after deploying memory protection.
     */
    ForeachNodeSafe(&task->tc_MemEntry, mb, mbnext)
    {
        DREMTASK("RemTask freeing MemList 0x%p", mb);
        /* Free one MemList node */
        FreeEntry(mb);
    }

    /* Changing the task lists always needs a Disable(). */
    Disable();

    /* Freeing myself? */
    if(task==SysBase->ThisTask)
    {

        /*
            Since I don't know how many levels of Forbid()
            are already pending I set a default value.
        */
        SysBase->TDNestCnt = -1;

        /* And force a task switch. Note: Dispatch, not Switch,
           because the state of thistask must not be saved ->
           after all the mem for the task + intetask + context
           could already have been freed by the FreeEntry() call
           above!!! */
           
        KrnDispatch();
        /* Does not return. */
    }

    /* All done. */
    Enable();
    Permit();

    DREMTASK("Success");

    AROS_LIBFUNC_EXIT
}


