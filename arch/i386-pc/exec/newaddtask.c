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
#include <proto/exec.h>
#include "etask.h"
#include "exec_util.h"

#include "exec_debug.h"
#ifndef DEBUG_AddTask
#   define DEBUG_AddTask 0
#endif
#undef DEBUG
#if DEBUG_AddTask
#   define DEBUG 1
#endif
#include <aros/debug.h>

/* if #define fills the unused stack with 0xE1 */
#define STACKSNOOP

void AROS_SLIB_ENTRY(TrapHandler,Exec)(void);

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
	Certain task fields should be intitialized and a stack must be
	allocated before calling this function. tc_SPReg will be used as the
	starting location for the stack pointer, i.e. a part of the stack can
	be reserved to pass the task some initial arguments.
	Memory can be added to the tc_MemEntry list and will be freed when the
	task dies. The new task's registers are set to 0.

    INPUTS
	task	  - Pointer to task structure.
	initialPC - Entry point for the new task.
	finalPC   - Routine that is called if the initialPC() function returns.
		    A NULL pointer installs the default finalizer.

    RESULT
	The address of the new task or NULL if the operation failed (can only
	happen with TF_ETASK set - currenty not implemented).

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	RemTask()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    /* APTR sp; */

    D(bug("Call NewAddTask (%08lx (\"%s\"), %08lx, %08lx)\n"
	, task
	, task->tc_Node.ln_Name
	, initialPC
	, finalPC
    ));
    ASSERT_VALID_PTR(task);

    /* Set node type to NT_TASK if not set to something else. */
    if(!task->tc_Node.ln_Type)
	task->tc_Node.ln_Type=NT_TASK;

    /* Sigh - you should provide a name for your task. */
    if(task->tc_Node.ln_Name==NULL)
	task->tc_Node.ln_Name="unknown task";

    /* This is moved into SysBase at the tasks's startup */
    task->tc_IDNestCnt=-1;
    task->tc_TDNestCnt=-1;

    task->tc_State = TS_ADDED;
    task->tc_Flags = 0;
    
    task->tc_SigWait = 0;
    task->tc_SigRecvd = 0;
    task->tc_SigExcept = 0;
        
    /* Signals default to all system signals allocated. */
    if(task->tc_SigAlloc==0)
	task->tc_SigAlloc=SysBase->TaskSigAlloc;

    /* Currently only used for segmentation violation */
    if(task->tc_TrapCode==NULL)
	task->tc_TrapCode=SysBase->TaskTrapCode;

    if(task->tc_ExceptCode==NULL)
	task->tc_ExceptCode=SysBase->TaskExceptCode;
	
#if !(AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
    /*
	If you can't to store the registers on the signal stack, you
	must set this flag.
    */
    task->tc_Flags |= TF_ETASK;
#endif

    /* Allocate the ETask structure if requested */
    if (task->tc_Flags & TF_ETASK)
    {
	task->tc_UnionETask.tc_ETask = AllocTaskMem (task
	    , sizeof (struct IntETask)
	    , MEMF_ANY|MEMF_CLEAR
	);

	if (!task->tc_UnionETask.tc_ETask)
	    return NULL;

	/* I'm the parent task */
	GetETask(task)->et_Parent = FindTask(NULL);
    }

    /* Get new stackpointer. */
    /* sp=task->tc_SPReg; */
    if (task->tc_SPReg==NULL)
#if AROS_STACK_GROWS_DOWNWARDS
	task->tc_SPReg = (UBYTE *)(task->tc_SPUpper) - SP_OFFSET;
#else
	task->tc_SPReg = (UBYTE *)(task->tc_SPLower) - SP_OFFSET;
#endif

#ifdef STACKSNOOP
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
    if(finalPC==NULL)
	finalPC=SysBase->TaskExitCode;

    /* Init new context. */
    if (!PrepareContext (task, initialPC, finalPC, tagList))
    {
	FreeTaskMem (task, task->tc_UnionETask.tc_ETask);
	return NULL;
    }

    /* store sp */
    /* task->tc_SPReg=sp; */

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
	if(SysBase->TDNestCnt>=0||SysBase->IDNestCnt>0)
	    /* No. Store it for later. */
	    SysBase->AttnResched|=0x80;
	else
	{
	    /* Switches are allowed. Move the current task away. */
	    //SysBase->ThisTask->tc_State=TS_READY;
	    //Enqueue(&SysBase->TaskReady,&SysBase->ThisTask->tc_Node);

	    /* And force a reschedule. */
	    Reschedule(task);
//	    Supervisor(Exec_Switch);   //Switch();
	}
    }

/*    if(SysBase->TaskReady.lh_Head->ln_Succ == task)
	Reschedule(task);*/
    Enable();

    ReturnPtr ("NewAddTask", struct Task *, task);
    AROS_LIBFUNC_EXIT
} /* NewAddTask */
