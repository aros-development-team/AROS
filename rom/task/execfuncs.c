/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "taskres_intern.h"

extern struct TaskResBase *internTaskResBase;

void TaskResAddTask(struct Task *task)
{
    struct TaskListEntry *newEntry;

    /*
       TODO:
        if the list is locked, defer tasks addition until it is unlocked
    */
    if ((newEntry = AllocMem(sizeof(struct TaskListEntry), MEMF_CLEAR)) != NULL)
    {
        D(bug("[TaskRes] TaskResAddTask: taskentry @ 0x%p\n", newEntry));
        newEntry->tle_Task = task;
        AddTail(&internTaskResBase->trb_TaskList, &newEntry->tle_Node);
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

    D(bug("[TaskRes] NewAddTask()\n"));

    newTask = AROS_CALL4(APTR, internTaskResBase->trb_NewAddTask,
                AROS_LCA(struct Task *,     task,      A1),
                AROS_LCA(APTR,              initialPC, A2),
                AROS_LCA(APTR,              finalPC,   A3),
                AROS_LCA(struct TagItem *,  tagList,   A4),
		struct ExecBase *, SysBase);

    D(bug("[TaskRes] NewAddTask: task @ 0x%p\n", task));

    if (newTask)
        TaskResAddTask(newTask);

    return newTask;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, RemTask,
        AROS_LHA(struct Task *, task, A1),
        struct ExecBase *, SysBase, 48, Task)
{
    AROS_LIBFUNC_INIT

    struct TaskListEntry *tmpEntry;

    D(bug("[TaskRes] RemTask()\n"));

    AROS_CALL1(APTR, internTaskResBase->trb_RemTask,
                AROS_LCA(struct Task *,     task,      A1),
		struct ExecBase *, SysBase);

    ForeachNode(&internTaskResBase->trb_TaskList, tmpEntry)
    {
        if (tmpEntry->tle_Task == task)
        {
            /*
               TODO:
                if the list is locked flag the entry to be removed,
                else remove it immediately
            */
            Remove(&tmpEntry->tle_Node);
            FreeMem(tmpEntry, sizeof(struct TaskListEntry));
            break;
        }
    }

    return;

    AROS_LIBFUNC_EXIT
}
