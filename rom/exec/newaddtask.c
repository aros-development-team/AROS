/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a task.
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <aros/libcall.h>
#include <aros/config.h>
#include <proto/exec.h>
#include "etask.h"
#include "exec_util.h"

#include "exec_debug.h"
#ifndef DEBUG_NewAddTask
#   define DEBUG_NewAddTask 0
#endif
#undef DEBUG
#if DEBUG_NewAddTask
#   define DEBUG 1
#endif
#include <aros/debug.h>

/*****************************************************************************

    NAME */

	AROS_LH4(APTR, NewAddTask,

/*  SYNOPSIS */
	AROS_LHA(struct Task *,     task,      A1),
	AROS_LHA(APTR,              initialPC, A2),
	AROS_LHA(APTR,              finalPC,   A3),
	AROS_LHA(struct TagItem *,  tagList,   A4),

/*  LOCATION */
	struct ExecBase *, SysBase, 152, Exec)

/*  FUNCTION
	Add a new task to the system. If the new task has the highest
	priority of all and task switches are allowed it will be started
	immediately.

	You must initialise certain fields, and allocate a stack before
	calling this function. The fields that must be initialised are:
	tc_SPLower, tc_SPUpper, tc_SPReg, and the tc_Node structure.

	If any other fields are initialised to zero, then they will be
	filled in with the system defaults.

	The value of tc_SPReg will be used as the location for the stack
	pointer. You can place any arguments you wish to pass to the Task
	onto the stack before calling AddTask(). However note that you may
	need to consider the stack direction on your processor.

	Memory can be added to the tc_MemEntry list and will be freed when
	the task dies. The new task's registers are set to 0.

    INPUTS
	task	  - Pointer to task structure.
	initialPC - Entry point for the new task.
	finalPC   - Routine that is called if the initialPC() function
		    returns. A NULL pointer installs the default finalizer.

    RESULT
	The address of the new task or NULL if the operation failed.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	RemTask()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("Call NewAddTask (%08lx (\"%s\"), %08lx, %08lx)\n"
	, task
	, task->tc_Node.ln_Name
	, initialPC
	, finalPC
    ));
    ASSERT_VALID_PTR(task);

    /* Set node type to NT_TASK if not set to something else. */
    if(task->tc_Node.ln_Type == 0)
	task->tc_Node.ln_Type = NT_TASK;

    /* Sigh - you should provide a name for your task. */
    if(task->tc_Node.ln_Name == NULL)
	task->tc_Node.ln_Name = "unknown task";

    /* This is moved into SysBase at the tasks's startup */
    task->tc_IDNestCnt=-1;
    task->tc_TDNestCnt=-1;

    task->tc_State = TS_ADDED;
    task->tc_Flags = 0;
    
    task->tc_SigWait = 0;
    task->tc_SigRecvd = 0;
    task->tc_SigExcept = 0;

    /* Signals default to all system signals allocated. */
    if(task->tc_SigAlloc == 0)
	task->tc_SigAlloc = SysBase->TaskSigAlloc;

    /*
     *	tc_SigWait, tc_SigRecvd, tc_SigExcept are default 0
     *	tc_ExceptCode, tc_ExceptData default to NULL.
     */

    /* Currently only used for segmentation violation */
    if(task->tc_TrapCode == NULL)
	task->tc_TrapCode = SysBase->TaskTrapCode;

    if (task->tc_ExceptCode == NULL)
        task->tc_ExceptCode=SysBase->TaskExceptCode;

    /*
	If you can't to store the registers on the signal stack, you
	must set this flag.
    */
    task->tc_Flags |= TF_ETASK;

    /* Allocate the ETask structure if requested */
    if (task->tc_Flags & TF_ETASK)
    {
	struct ETask *et;

	/*
	 *  We don't add this to the task memory, it isn't free'd by
	 *  RemTask(), rather by somebody else calling ChildFree().
	 *  Alternatively, an orphaned task will free its own ETask.
	 */
	task->tc_UnionETask.tc_ETask = AllocVec
	(
	    sizeof (struct IntETask),
	    MEMF_ANY|MEMF_CLEAR
	);

	if (!task->tc_UnionETask.tc_ETask)
	    return NULL;

	et = (struct ETask *)task->tc_UnionETask.tc_ETask;
	et->et_Parent = FindTask(NULL);
	NEWLIST(&et->et_Children);

	/* Initialise the message list */
	NEWLIST(&et->et_TaskMsgPort.mp_MsgList);
	et->et_TaskMsgPort.mp_Flags = PA_SIGNAL;
	et->et_TaskMsgPort.mp_Node.ln_Type = NT_MSGPORT;
	et->et_TaskMsgPort.mp_SigTask = task;
	et->et_TaskMsgPort.mp_SigBit = SIGB_CHILD;

	/* Initialise the trap fields */
	et->et_TrapAlloc = SysBase->TaskTrapAlloc;
	et->et_TrapAble = 0;

#if 0
	Forbid();
	while(et->et_UniqueID == 0)
	{
	    /*
	     *	Add some fuzz on wrapping. Its likely that the early numbers
	     *	where taken by somebody else.
	     */
	    if(++SysBase->ex_TaskID == 0)
		SysBase->exTaskID = 1024;

	    Disable();
	    if(FindTaskByID(SysBase->ex_TaskID, SysBase) == NULL)
		et->et_UniqueID = SysBase->ex_TaskID;
	    Enable();
	}
	Permit();
#endif
    }
    else
    {
	task->tc_UnionETask.tc_ETrap.tc_ETrapAlloc = SysBase->TaskTrapAlloc;
	task->tc_UnionETask.tc_ETrap.tc_ETrapAble = 0;
    }

    /* Get new stackpointer. Note, the doc says this MUST be initialised. */
    if (task->tc_SPReg==NULL)
#if AROS_STACK_GROWS_DOWNWARDS
	task->tc_SPReg = (UBYTE *)(task->tc_SPUpper) - SP_OFFSET;
#else
	task->tc_SPReg = (UBYTE *)(task->tc_SPLower) - SP_OFFSET;
#endif

#if AROS_STACK_DEBUG
    {
        UBYTE *startfill, *endfill;

    #if AROS_STACK_GROWS_DOWNWARDS
	startfill = (UBYTE *)task->tc_SPLower;
	endfill   = ((UBYTE *)task->tc_SPReg) - 16;
    #else
        startfill = ((UBYTE *)task->tc_SPReg) + 16;
	endfill   = ((UBYTE *)task->tc_SPUpper) - 1; /* FIXME: -1 correct ?? */
    #endif

        while(startfill <= endfill)
	{
	    *startfill++ = 0xE1;
	}

    }
#endif

    /* Default finalizer? */
    if(finalPC == NULL)
	finalPC = SysBase->TaskExitCode;

    /* Init new context. */
    if (!PrepareContext (task, initialPC, finalPC, tagList))
    {
	FreeVec(task->tc_UnionETask.tc_ETask);
	return NULL;
    }

    /* Set the task flags for switch and launch. */
    if(task->tc_Switch)
	task->tc_Flags|=TF_SWITCH;

    if(task->tc_Launch)
	task->tc_Flags|=TF_LAUNCH;

    /* tc_MemEntry _must_ already be set. */

    /*
	Protect the task lists. This must be done with Disable() because
	of Signal() which is usable from interrupts and may change those
	lists.
     */
    Disable();

    /* Add the new task to the ready list. */
    task->tc_State=TS_READY;
    Enqueue(&SysBase->TaskReady,&task->tc_Node);

    /*
	Determine if a task switch is necessary. (If the new task has a
	higher priority than the current one and the current one
	is still active.) If the current task isn't of type TS_RUN it
	is already gone.
    */
    if(task->tc_Node.ln_Pri>SysBase->ThisTask->tc_Node.ln_Pri&&
       SysBase->ThisTask->tc_State==TS_RUN)
    {
	/* Are taskswitches allowed? (Don't count own Disable() here) */
	if(SysBase->TDNestCnt>=0||SysBase->IDNestCnt>0) {
	    /* No. Store it for later. */
	    SysBase->AttnResched|=0x80;
	} else {
	    /* Switches are allowed. Move the current task away. */
	    SysBase->ThisTask->tc_State=TS_READY;
	    Enqueue(&SysBase->TaskReady,&SysBase->ThisTask->tc_Node);

	    /* And force a rescedule. */
	    Switch();
	}
    }

    Enable();

    ReturnPtr ("NewAddTask", struct Task *, task);
    AROS_LIBFUNC_EXIT
} /* NewAddTask */

/* Default finaliser. */
AROS_UFH1(void, Exec_TaskFinaliser,
	  AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    /* Get rid of current task. */
    RemTask(SysBase->ThisTask);
    
    AROS_USERFUNC_EXIT
}
