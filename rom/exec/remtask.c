/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:27:16  digulla
    Added copyright notics and made headers conform

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(void, RemTask,

/*  SYNOPSIS */
	__AROS_LA(struct Task *,     task, A1),

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
    __AROS_FUNC_INIT

    struct MemList *mb;

    /* A value of NULL means current task */
    if(task==NULL)
	task=SysBase->ThisTask;

    /*
	Since it's possible that the following will free a task
	structure that is used for some time afterwards it's
	necessary to protect the free memory list so that nobody
	can allocate that memory.
    */
    Forbid();

    /* Free all memory in the tc_MemEntry list. */
    while((mb=(struct MemList *)RemHead(&task->tc_MemEntry))!=NULL)
	/* Free one MemList node */
	FreeEntry(mb);

    /* Changing the task lists always needs a Disable(). */
    Disable();

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
    }else
	/* Good luck. Freeing other tasks is simple. */
	Remove(&task->tc_Node);

    /* All done. */
    Enable();
    Permit();
    __AROS_FUNC_EXIT
}


