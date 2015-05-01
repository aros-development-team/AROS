/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <resources/task.h>

#include <exec_intern.h>

#include "taskres_intern.h"

extern APTR AROS_SLIB_ENTRY(AddTask, Task, 47);
extern APTR AROS_SLIB_ENTRY(NewAddTask, Task, 176);
extern APTR AROS_SLIB_ENTRY(RemTask, Task, 48);

struct TaskResBase *internTaskResBase = NULL;

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

    internTaskResBase = TaskResBase;

    NEWLIST(&TaskResBase->trb_TaskList);
    NEWLIST(&TaskResBase->trb_LockedLists);

    TaskResBase->trb_AddTask = SetFunction((struct Library *)SysBase, -47*LIB_VECTSIZE, AROS_SLIB_ENTRY(AddTask, Task, 47));
    TaskResBase->trb_NewAddTask = SetFunction((struct Library *)SysBase, -176*LIB_VECTSIZE, AROS_SLIB_ENTRY(NewAddTask, Task, 176));
    TaskResBase->trb_RemTask = SetFunction((struct Library *)SysBase, -48*LIB_VECTSIZE, AROS_SLIB_ENTRY(RemTask, Task, 48));

    /*
       Add existing tasks to our internal list ..
    */
#if defined(__AROSEXEC_SMP__)
    listLock = KrnSpinLock(&PrivExecBase(SysBase)->TaskRunningSpinLock, SPINLOCK_MODE_READ);
    ForeachNode(&PrivExecBase(SysBase)->TaskRunning, curTask)
    {
        if ((taskEntry = AllocMem(sizeof(struct TaskListEntry), MEMF_CLEAR)) != NULL)
        {
            taskEntry->tle_Task = curTask;
            AddTail(&TaskResBase->trb_TaskList, &taskEntry->tle_Node);
        }
    }
    KrnSpinUnLock(listLock);
    listLock = KrnSpinLock(&PrivExecBase(SysBase)->TaskReadySpinLock, SPINLOCK_MODE_READ);
#else
    if (SysBase->ThisTask)
    {
        if ((taskEntry = AllocMem(sizeof(struct TaskListEntry), MEMF_CLEAR)) != NULL)
        {
            taskEntry->tle_Task = SysBase->ThisTask;
            AddTail(&TaskResBase->trb_TaskList, &taskEntry->tle_Node);
        }
    }
#endif
    ForeachNode(&SysBase->TaskReady, curTask)
    {
        if ((taskEntry = AllocMem(sizeof(struct TaskListEntry), MEMF_CLEAR)) != NULL)
        {
            taskEntry->tle_Task = curTask;
            AddTail(&TaskResBase->trb_TaskList, &taskEntry->tle_Node);
        }
    }
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(listLock);
    listLock = KrnSpinLock(&PrivExecBase(SysBase)->TaskWaitSpinLock, SPINLOCK_MODE_READ);
#endif
    ForeachNode(&SysBase->TaskWait, curTask)
    {
        if ((taskEntry = AllocMem(sizeof(struct TaskListEntry), MEMF_CLEAR)) != NULL)
        {
            taskEntry->tle_Task = curTask;
            AddTail(&TaskResBase->trb_TaskList, &taskEntry->tle_Node);
        }
    }
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(listLock);
#endif

    return TRUE;
}

ADD2INITLIB(taskres_Init, 0)
