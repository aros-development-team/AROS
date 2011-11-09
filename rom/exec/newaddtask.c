/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a task.
    Lang: english
*/

#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <aros/debug.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "etask.h"
#include "exec_util.h"
#include "exec_debug.h"
#include "taskstorage.h"

/*****************************************************************************

    NAME */

        AROS_LH4(APTR, NewAddTask,

/*  SYNOPSIS */
        AROS_LHA(struct Task *,     task,      A1),
        AROS_LHA(APTR,              initialPC, A2),
        AROS_LHA(APTR,              finalPC,   A3),
        AROS_LHA(struct TagItem *,  tagList,   A4),

/*  LOCATION */
        struct ExecBase *, SysBase, 176, Exec)

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
        task      - Pointer to task structure.
        initialPC - Entry point for the new task.
        finalPC   - Routine that is called if the initialPC() function returns.
                    A NULL pointer installs the default finalizer.

    RESULT
        The address of the new task or NULL if the operation failed (can only
        happen with TF_ETASK set - currenty not implemented).

    NOTES
    	This function is private. Use MorphOS-compatible NewCreateTaskA()
    	in your applications.

    EXAMPLE

    BUGS

    SEE ALSO
        RemTask()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ASSERT_VALID_PTR(task);

    /* Sigh - you should provide a name for your task. */
    if(task->tc_Node.ln_Name==NULL)
        task->tc_Node.ln_Name="unknown task";

    DADDTASK("NewAddTask (0x%p (\"%s\"), 0x%p, 0x%p)", task, task->tc_Node.ln_Name, initialPC, finalPC);

    /* Initialize the memory entry list if the caller forgot */
    if (!task->tc_MemEntry.lh_Head)
        NEWLIST(&task->tc_MemEntry);

    DADDTASK("NewAddTask MemEntry head: 0x%p", GetHead(&task->tc_MemEntry.lh_Head));

    /* Set node type to NT_TASK if not set to something else. */
    if(!task->tc_Node.ln_Type)
        task->tc_Node.ln_Type=NT_TASK;

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
        
    /*
     * EXECF_StackSnoop can be set or reset at runtime.
     * However task's stack is either snooped or not, it's problematic
     * to turn it on at runtime. So we initialize it when the task starts up.
     */
    if (PrivExecBase(SysBase)->IntFlags & EXECF_StackSnoop)
    	task->tc_Flags |= TF_STACKCHK;

    /* Initialize ETask */
    InitETask(task);
    if (!task->tc_UnionETask.tc_ETask)
        return NULL;

    /* Get new stackpointer. */
    if (task->tc_SPReg==NULL)
        task->tc_SPReg = (UBYTE *)(task->tc_SPUpper) - SP_OFFSET;

#ifdef AROS_STACKALIGN
    if ((IPTR)task->tc_SPReg & (AROS_STACKALIGN - 1))
    {
        DADDTASK("NewAddTask with unaligned stack pointer (0x%p)! Fixing...", task->tc_SPReg);
    	task->tc_SPReg = (APTR)((IPTR)task->tc_SPReg & ~(AROS_STACKALIGN - 1));
    }
#endif
    DADDTASK("NewAddTask: SPLower: 0x%p SPUpper: 0x%p SP: 0x%p", task->tc_SPLower, task->tc_SPUpper, task->tc_SPReg);

    if (task->tc_Flags & TF_STACKCHK)
    {
        UBYTE *startfill, *endfill;

        startfill = (UBYTE *)task->tc_SPLower;
        endfill   = ((UBYTE *)task->tc_SPReg) - 16;

        while(startfill <= endfill)
        {
            *startfill++ = 0xE1;
        }
    }

    /* Default finalizer? */
    if(finalPC==NULL)
        finalPC=SysBase->TaskExitCode;

    /* Init new context. */
    if (!PrepareContext (task, initialPC, finalPC, tagList))
    {
        FreeTaskMem (task, task->tc_UnionETask.tc_ETask);
        return NULL;
    }

    /* Set the task flags for switch and launch. */
    if(task->tc_Switch)
        task->tc_Flags|=TF_SWITCH;

    if(task->tc_Launch)
        task->tc_Flags|=TF_LAUNCH;

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

    if (task->tc_Node.ln_Pri > SysBase->ThisTask->tc_Node.ln_Pri &&
       SysBase->ThisTask->tc_State == TS_RUN)
    {
        D(bug("[AddTask] Rescheduling...\n"));

        /* Reschedule() will take care about disabled task switching automatically */
    	Reschedule(task);
    }

    Enable();

    DADDTASK("Added task 0x%p", task);
    return task;

    AROS_LIBFUNC_EXIT
} /* NewAddTask */
