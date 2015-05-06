/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

#include <aros/debug.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <resources/task.h>

#include <exec_intern.h>

#include "taskres_intern.h"

extern APTR AROS_SLIB_ENTRY(NewAddTask, Task, 176)();
extern void AROS_SLIB_ENTRY(RemTask, Task, 48)();

static LONG taskres_Init(struct TaskResBase *TaskResBase)
{
#if defined(__AROSEXEC_SMP__)
    spinlock_t *listLock;
#endif
    struct TaskListEntry *taskEntry = NULL;
    struct Task *curTask = NULL;

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
    	return FALSE;

    TaskResBase->trb_UtilityBase = OpenLibrary("utility.library", 0);
    if (!TaskResBase->trb_UtilityBase)
        return FALSE;

    NEWLIST(&TaskResBase->trb_TaskList);
    NEWLIST(&TaskResBase->trb_LockedLists);

    SysBase->lb_TaskResBase = (struct Library *)TaskResBase;

    /*
        Use Disable/Enable to lock all access to the task list -:
        the SpinLocks should only be used by the schedular itself, otherwise we
        may end up with a deadlock.
    */
    Disable();

    TaskResBase->trb_NewAddTask = SetFunction((struct Library *)SysBase, -176*LIB_VECTSIZE, AROS_SLIB_ENTRY(NewAddTask, Task, 176));
    TaskResBase->trb_RemTask = SetFunction((struct Library *)SysBase, -48*LIB_VECTSIZE, AROS_SLIB_ENTRY(RemTask, Task, 48));

    /*
       Add existing tasks to our internal list ..
    */
#if defined(__AROSEXEC_SMP__)
    ForeachNode(&PrivExecBase(SysBase)->TaskRunning, curTask)
    {
        if ((taskEntry = AllocMem(sizeof(struct TaskListEntry), MEMF_CLEAR)) != NULL)
        {
            D(bug("[TaskRes] 0x%p [R  ] %s\n", curTask, curTask->tc_Node.ln_Name));
            taskEntry->tle_Task = curTask;
            AddTail(&TaskResBase->trb_TaskList, &taskEntry->tle_Node);
        }
    }
#else
    if (SysBase->ThisTask)
    {
        if ((taskEntry = AllocMem(sizeof(struct TaskListEntry), MEMF_CLEAR)) != NULL)
        {
            D(bug("[TaskRes] 0x%p [R  ] %s\n", SysBase->ThisTask, SysBase->ThisTask->tc_Node.ln_Name));
            taskEntry->tle_Task = SysBase->ThisTask;
            AddTail(&TaskResBase->trb_TaskList, &taskEntry->tle_Node);
        }
    }
#endif
    ForeachNode(&SysBase->TaskReady, curTask)
    {
        if ((taskEntry = AllocMem(sizeof(struct TaskListEntry), MEMF_CLEAR)) != NULL)
        {
            D(bug("[TaskRes] 0x%p [ R ] %s\n", curTask, curTask->tc_Node.ln_Name));
            taskEntry->tle_Task = curTask;
            AddTail(&TaskResBase->trb_TaskList, &taskEntry->tle_Node);
        }
    }
    ForeachNode(&SysBase->TaskWait, curTask)
    {
        if ((taskEntry = AllocMem(sizeof(struct TaskListEntry), MEMF_CLEAR)) != NULL)
        {
            D(bug("[TaskRes] 0x%p [  W] %s\n", curTask, curTask->tc_Node.ln_Name));
            taskEntry->tle_Task = curTask;
            AddTail(&TaskResBase->trb_TaskList, &taskEntry->tle_Node);
        }
    }

    Enable();

    return TRUE;
}

static LONG taskres_Exit(struct TaskResBase *TaskResBase)
{
    SetFunction((struct Library *)SysBase, -176*LIB_VECTSIZE, TaskResBase->trb_NewAddTask);
    SetFunction((struct Library *)SysBase, -48*LIB_VECTSIZE, TaskResBase->trb_RemTask);

    CloseLibrary(TaskResBase->trb_UtilityBase);

    return TRUE;
}

ADD2INITLIB(taskres_Init, 0)

ADD2EXPUNGELIB(taskres_Exit, 0)
