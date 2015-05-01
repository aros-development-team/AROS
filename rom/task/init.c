/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "taskres_intern.h"

extern APTR AROS_SLIB_ENTRY(AddTask, Task, 47);
extern APTR AROS_SLIB_ENTRY(NewAddTask, Task, 176);
extern APTR AROS_SLIB_ENTRY(RemTask, Task, 48);

struct TaskResBase *internTaskResBase = NULL;

static LONG taskres_Init(struct TaskResBase *TaskResBase)
{
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
       TODO: get the running tasks from all cores ..
    */

    ForeachNode(&SysBase->TaskReady, curTask)
    {
        if ((taskEntry = AllocMem(sizeof(struct TaskListEntry), MEMF_CLEAR)) != NULL)
        {
            taskEntry->tle_Task = curTask;
            AddTail(&TaskResBase->trb_TaskList, &taskEntry->tle_Node);
        }
    }
    ForeachNode(&SysBase->TaskWait, curTask)
    {
        if ((taskEntry = AllocMem(sizeof(struct TaskListEntry), MEMF_CLEAR)) != NULL)
        {
            taskEntry->tle_Task = curTask;
            AddTail(&TaskResBase->trb_TaskList, &taskEntry->tle_Node);
        }
    }

    return TRUE;
}

ADD2INITLIB(taskres_Init, 0)
