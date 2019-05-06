/*
    Copyright © 2015-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <proto/execlock.h>
#include <resources/execlock.h>
#include <resources/task.h>

#include "etask.h"

#include "task_intern.h"

extern APTR AROS_SLIB_ENTRY(NewAddTask, Task, 176)();
extern void AROS_SLIB_ENTRY(RemTask, Task, 48)();

static LONG taskres_Init(struct TaskResBase *TaskResBase)
{
#ifdef TASKRES_ENABLE
    struct TaskListEntry *taskEntry = NULL;
    struct Task *curTask = NULL;

#if defined(__AROSEXEC_SMP__)
    void *ExecLockBase = NULL;
#endif
#endif /* TASKRES_ENABLE */

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
    	return FALSE;

#if defined(__AROSEXEC_SMP__)
    TaskResBase->trb_ExecLock = OpenResource("execlock.resource");
    ExecLockBase = TaskResBase->trb_ExecLock;
#endif

    TaskResBase->trb_UtilityBase = OpenLibrary("utility.library", 0);
    if (!TaskResBase->trb_UtilityBase)
        return FALSE;

    NEWLIST(&TaskResBase->trb_TaskList);
    NEWLIST(&TaskResBase->trb_NewTasks);
    NEWLIST(&TaskResBase->trb_LockedLists);

    SysBase->lb_TaskResBase = (struct Library *)TaskResBase;

    InitSemaphore(&TaskResBase->trb_Sem);

#ifdef TASKRES_ENABLE
    TaskResBase->trb_RemTask = SetFunction((struct Library *)SysBase, -48*LIB_VECTSIZE, AROS_SLIB_ENTRY(RemTask, Task, 48));
    TaskResBase->trb_NewAddTask = SetFunction((struct Library *)SysBase, -176*LIB_VECTSIZE, AROS_SLIB_ENTRY(NewAddTask, Task, 176));

    /*
       Add existing tasks to our internal list ..
    */
#if defined(__AROSEXEC_SMP__)
    ObtainSystemLock(&PrivExecBase(SysBase)->TaskRunning, SPINLOCK_MODE_READ, LOCKF_DISABLE);
    ForeachNode(&PrivExecBase(SysBase)->TaskRunning, curTask)
    {
        if (curTask->tc_State & TS_RUN)
        {
            if ((taskEntry = AllocMem(sizeof(struct TaskListEntry), MEMF_CLEAR)) != NULL)
            {
                D(bug("[TaskRes] 0x%p [R  ] %02d %s\n", curTask, GetIntETask(curTask)->iet_CpuNumber, curTask->tc_Node.ln_Name));
                NEWLIST(&taskEntry->tle_HookTypes);
                taskEntry->tle_Task = curTask;
                AddTail(&TaskResBase->trb_TaskList, &taskEntry->tle_Node);
            }
            else
            {
                bug("[TaskRes] Failed to allocate storage for task @  0x%p!!\n", curTask);
            }
        }
        else
        {
            bug("[TaskRes] Invalid Task State %08x for task @ 0x%p\n", curTask->tc_State, curTask);
        }
    }
    ReleaseSystemLock(&PrivExecBase(SysBase)->TaskRunning, LOCKF_DISABLE);

    ObtainSystemLock(&PrivExecBase(SysBase)->TaskSpinning, SPINLOCK_MODE_READ, LOCKF_DISABLE);
    ForeachNode(&PrivExecBase(SysBase)->TaskSpinning, curTask)
    {
        if (curTask->tc_State & TS_SPIN)
        {
            if ((taskEntry = AllocMem(sizeof(struct TaskListEntry), MEMF_CLEAR)) != NULL)
            {
                D(bug("[TaskRes] 0x%p [  S] %02d %s\n", curTask, GetIntETask(curTask)->iet_CpuNumber, curTask->tc_Node.ln_Name));
                taskEntry->tle_Task = curTask;
                AddTail(&TaskResBase->trb_TaskList, &taskEntry->tle_Node);
            }
            else
            {
                bug("[TaskRes] Failed to allocate storage for task @  0x%p!!\n", curTask);
            }
        }
        else
        {
            bug("[TaskRes] Invalid Task State %08x for task @ 0x%p\n", curTask->tc_State, curTask);
        }
    }
    ReleaseSystemLock(&PrivExecBase(SysBase)->TaskSpinning, LOCKF_DISABLE);

    ObtainSystemLock(&SysBase->TaskReady, SPINLOCK_MODE_READ, LOCKF_DISABLE);
    // TODO : list TaskSpinning tasks..
#else
    Disable();
    if (SysBase->ThisTask)
    {
        if (SysBase->ThisTask->tc_State & TS_RUN)
        {
            if ((taskEntry = AllocMem(sizeof(struct TaskListEntry), MEMF_CLEAR)) != NULL)
            {
                D(bug("[TaskRes] 0x%p [R--] 00 %s\n", SysBase->ThisTask, SysBase->ThisTask->tc_Node.ln_Name));
                taskEntry->tle_Task = SysBase->ThisTask;
                AddTail(&TaskResBase->trb_TaskList, &taskEntry->tle_Node);
            }
            else
            {
                bug("[TaskRes] Failed to allocate storage for task @  0x%p!!\n", SysBase->ThisTask);
            }
        }
        else
        {
            bug("[TaskRes] Invalid Task State %08x for task @ 0x%p\n", SysBase->ThisTask->tc_State, curTask);
        }
    }
#endif
    ForeachNode(&SysBase->TaskReady, curTask)
    {
        if (curTask->tc_State & (TS_READY|TS_RUN))
        {
            if ((taskEntry = AllocMem(sizeof(struct TaskListEntry), MEMF_CLEAR)) != NULL)
            {
                D(bug("[TaskRes] 0x%p [-R-] -- %s\n", curTask, curTask->tc_Node.ln_Name));
                taskEntry->tle_Task = curTask;
                AddTail(&TaskResBase->trb_TaskList, &taskEntry->tle_Node);
            }
            else
            {
                bug("[TaskRes] Failed to allocate storage for task @  0x%p!!\n", curTask);
            }
        }
        else
        {
            bug("[TaskRes] Invalid Task State %08x for task @ 0x%p\n", curTask->tc_State, curTask);
        }
    }
#if defined(__AROSEXEC_SMP__)
    ReleaseSystemLock(&SysBase->TaskReady, LOCKF_DISABLE);

    ObtainSystemLock(&SysBase->TaskWait, SPINLOCK_MODE_READ, LOCKF_DISABLE);
#endif
    ForeachNode(&SysBase->TaskWait, curTask)
    {
        if (curTask->tc_State & TS_WAIT)
        {
            if ((taskEntry = AllocMem(sizeof(struct TaskListEntry), MEMF_CLEAR)) != NULL)
            {
                D(bug("[TaskRes] 0x%p [--W] -- %s\n", curTask, curTask->tc_Node.ln_Name));
                taskEntry->tle_Task = curTask;
                AddTail(&TaskResBase->trb_TaskList, &taskEntry->tle_Node);
            }
            else
            {
                bug("[TaskRes] Failed to allocate storage for task @  0x%p!!\n", curTask);
            }
        }
        else
        {
            bug("[TaskRes] Invalid Task State %08x for task @ 0x%p\n", curTask->tc_State, curTask);
        }
    }
#if defined(__AROSEXEC_SMP__)
    ReleaseSystemLock(&SysBase->TaskWait, LOCKF_DISABLE);
#else
    Enable();
#endif

#endif /* TASKRES_ENABLE */

    return TRUE;
}

static LONG taskres_Exit(struct TaskResBase *TaskResBase)
{
#ifdef TASKRES_ENABLE
    SetFunction((struct Library *)SysBase, -176*LIB_VECTSIZE, TaskResBase->trb_NewAddTask);
    SetFunction((struct Library *)SysBase, -48*LIB_VECTSIZE, TaskResBase->trb_RemTask);
#endif /* TASKRES_ENABLE */

    CloseLibrary(TaskResBase->trb_UtilityBase);

    return TRUE;
}

ADD2INITLIB(taskres_Init, 0)

ADD2EXPUNGELIB(taskres_Exit, 0)
