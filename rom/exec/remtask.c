/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove a task
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_debug.h"
#ifndef DEBUG_RemTask
#   define DEBUG_RemTask 0
#endif
#undef DEBUG
#if DEBUG_RemTask
#   define DEBUG 1
#endif
#include <aros/debug.h>

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

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct MemList *mb;
    struct ETask *et;

    /* A value of NULL means current task */
    if (task==NULL)
	task=SysBase->ThisTask;

    D(bug("Call RemTask (%08lx (\"%s\"))\n", task, task->tc_Node.ln_Name));
    et = GetETask(task);

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

    /* Free all memory in the tc_MemEntry list. */
    while((mb=(struct MemList *)RemHead(&task->tc_MemEntry))!=NULL)
	/* Free one MemList node */
	FreeEntry(mb);

#if 0
    if(et != NULL)
    {
	struct ETask *child;

	/* Clean up after all the children that the task didn't do itself. */
	ForeachNode(&et->et_TaskMsgPort.mp_MsgList, child)
	{
	    /* This is effectively ChildFree() */
	    if(child->et_Result2)
		FreeVec(child->et_Result2);
	    FreeVec(child);
	}

	/* Orphan all our remaining children. */
	{
	    struct Task *save;

	    /* Do an effective ChildOrphan(0) */
	    ForeachNode(et->et_Children, child)
		child->et_Parent = NULL;
	}

	/* If we have an ETask parent, tell it we have exited. */
	if(et->et_Parent != NULL)
	{
	    child = GetETask(et->et_Parent);
	    if(child != NULL)
		PutMsg(&child->et_TaskMsgPort, et);
	    else
		FreeVec(et);
	}
	else
	    FreeVec(et);
    }
#else
    if(et != NULL)
    {
    	/* Orphan all our remaining children. */
	struct ETask *child;

        /* Do an effective ChildOrphan(0) */
	#warning FIXME: should we link the children to our parent?
        ForeachNode(&et->et_Children, child)
   	    child->et_Parent = NULL;

        /* If we have an ETask parent, unlink us from it */
        if(et->et_Parent != NULL)
        {
            REMOVE(et);
        }    
    }
#endif

    /* Changing the task lists always needs a Disable(). */
    Disable();

    FreeVec(et);
	
    /* Freeing myself? */
    if(task==SysBase->ThisTask)
    {
	/* Can't do that - let the dispatcher do it. */
	task->tc_State=TS_REMOVED;

	/*
	    Since I don't know how many levels of Forbid()
	    are already pending I set a default value.
	*/
	SysBase->TDNestCnt=-1;

	/* And force a task switch */
	Switch();
	/* Does not return. */
    }

    /* All done. */
    Enable();
    Permit();

    ReturnVoid ("RemTask");
    AROS_LIBFUNC_EXIT
} /* RemTask() */

