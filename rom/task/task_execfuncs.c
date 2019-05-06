/*
    Copyright © 2015-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>

#include "task_intern.h"

void TaskResAddTask(struct TaskResBase *TaskResBase, struct Task *task)
{
    struct TaskListEntry *newEntry;

    if ((newEntry = AllocMem(sizeof(struct TaskListEntry), MEMF_CLEAR)) != NULL)
    {
        D(bug("[TaskRes] TaskResAddTask: taskentry @ 0x%p for '%s'\n", newEntry, task->tc_Node.ln_Name));
        newEntry->tle_Task = task;
        NEWLIST(&newEntry->tle_HookTypes);
        if (IsListEmpty(&TaskResBase->trb_LockedLists))
            AddTail(&TaskResBase->trb_TaskList, &newEntry->tle_Node);
        else
            AddTail(&TaskResBase->trb_NewTasks, &newEntry->tle_Node);
    }
}

AROS_LH4(APTR, NewAddTask,
        AROS_LHA(struct Task *,     task,      A1),
        AROS_LHA(APTR,              initialPC, A2),
        AROS_LHA(APTR,              finalPC,   A3),
        AROS_LHA(struct TagItem *,  tagList,   A4),
        struct ExecBase *, SysBase, 176, Task)
{
    AROS_LIBFUNC_INIT

    APTR newTask;
    struct TaskResBase *TaskResBase;
    
    TaskResBase = (struct TaskResBase *)SysBase->lb_TaskResBase;

    D(bug("[TaskRes] NewAddTask(0x%p)\n", task));

    newTask = AROS_CALL4(APTR, TaskResBase->trb_NewAddTask,
                AROS_LCA(struct Task *,     task,      A1),
                AROS_LCA(APTR,              initialPC, A2),
                AROS_LCA(APTR,              finalPC,   A3),
                AROS_LCA(struct TagItem *,  tagList,   A4),
		struct ExecBase *, SysBase);

    D(bug("[TaskRes] NewAddTask: task @ 0x%p\n", newTask));

    if (newTask)
        TaskResAddTask(TaskResBase, newTask);

    return newTask;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, RemTask,
        AROS_LHA(struct Task *, task, A1),
        struct ExecBase *, SysBase, 48, Task)
{
    AROS_LIBFUNC_INIT

    struct Task *findTask = task;
    struct TaskListEntry *taskEntry, *tmpEntry;
    struct TaskResBase *TaskResBase;
    BOOL removed = FALSE;

    TaskResBase = (struct TaskResBase *)SysBase->lb_TaskResBase;

    if (!findTask)
        findTask = FindTask(NULL);

    D(
        bug("[TaskRes] RemTask(0x%p)\n", task);
        if (findTask != task)
            bug("[TaskRes] Real task @ 0x%p\n", findTask);
    )

    ForeachNodeSafe(&TaskResBase->trb_NewTasks, taskEntry, tmpEntry)
    {
        if (taskEntry->tle_Task == findTask)
        {
            D(bug("[TaskRes] RemTask: destroying new entry @ 0x%p\n", taskEntry));
            Remove(&taskEntry->tle_Node);
            FreeMem(taskEntry, sizeof(struct TaskListEntry));
            removed = TRUE;
            break;
        }
    }

    if (!removed)
    {
#if !defined(__AROSEXEC_SMP__)
        /* Don't let any other task interfere with us at the moment */
        Forbid();
#else
        EXEC_SPINLOCK_LOCK(&TaskResBase->TaskListSpinLock, NULL, SPINLOCK_MODE_WRITE);
#endif
        ForeachNodeSafe(&TaskResBase->trb_TaskList, taskEntry, tmpEntry)
        {
            if (taskEntry->tle_Task == findTask)
            {
                D(bug("[TaskRes] RemTask: taskentry @ 0x%p for '%s'\n", taskEntry, task->tc_Node.ln_Name));
                if (IsListEmpty(&TaskResBase->trb_LockedLists))
                {
                    D(bug("[TaskRes] RemTask: destroying entry\n"));
                    Remove(&taskEntry->tle_Node);
                    FreeMem(taskEntry, sizeof(struct TaskListEntry));
                }
                else
                {
                    D(bug("[TaskRes] RemTask: flag entry for removal\n"));
                    taskEntry->tle_Task = NULL;
                }
                break;
            }
        }
#if !defined(__AROSEXEC_SMP__)
        Permit();
#else
        EXEC_SPINLOCK_UNLOCK(&TaskResBase->TaskListSpinLock);
#endif
    }

    D(bug("[TaskRes] RemTask: Calling original Exec->RemTask()\n"));

    if (TaskResBase->trb_RemTask)
    {
        AROS_CALL1(void, TaskResBase->trb_RemTask,
                    AROS_LCA(struct Task *,     task,      A1),
                    struct ExecBase *, SysBase);
    }
    return;

    AROS_LIBFUNC_EXIT
}
