/*
    Copyright 2010-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/execbase.h>
#include <resources/task.h>
#include <clib/alib_protos.h>

#include <proto/task.h>
#include <proto/dos.h>

#include "locale.h"

#include "sysmon_intern.h"

//#define DEBUG 1
#include <aros/debug.h>


APTR TaskResBase = NULL;

/* Task information handling*/
struct TaskInfo
{
    struct Node TINode;
    struct Task *Task;
    ULONG TimeCurrent;
    ULONG TimeLast;
};

VOID UpdateTasksInformation(struct SysMonData * smdata)
{
    struct Task * task;
    IPTR firstvis = 0, entryid = 0;
    struct Task *selected = smdata->sm_TaskSelected;
    struct TaskList *systasklist;

    set(smdata->tasklist, MUIA_List_Quiet, TRUE);

    get(smdata->tasklist, MUIA_List_First, &firstvis);

    DoMethod(smdata->tasklist, MUIM_List_Clear);

    smdata->sm_TasksWaiting = 0;
    smdata->sm_TasksReady = 0;
    smdata->sm_TaskTotalRuntime = 0;

    systasklist = LockTaskList(0);
    while ((task = NextTaskEntry(systasklist, 0)) != NULL)
    {
        D(bug("[SysMon] task %s state %d\n", task->tc_Node.ln_Name, task->tc_State));
        
        if (task->tc_State == TS_READY)
        {
            smdata->sm_TasksReady++;
        }
        if ((task->tc_State == TS_WAIT) || (task->tc_State == TS_SPIN))
        {
            smdata->sm_TasksWaiting++;
        }

        entryid = DoMethod(smdata->tasklist, MUIM_List_InsertSingle, task, MUIV_List_Insert_Bottom);
        if (task == selected)
        {
            set(smdata->tasklist, MUIA_List_Active, entryid);
        }
    }
    UnLockTaskList(0);

    if (XGET(smdata->tasklist, MUIA_List_Active) == 0)
        smdata->sm_TaskSelected = NULL;

    /* Keep the "pre-refresh" view on List as much as possible */
    LONG v = (LONG)firstvis;
    if (firstvis + XGET(smdata->tasklist, MUIA_List_Visible) >= XGET(smdata->tasklist, MUIA_List_Entries))
    {
        v = XGET(smdata->tasklist, MUIA_List_Entries) - XGET(smdata->tasklist, MUIA_List_Visible);
        if (v < 0) v = 0;
    }

    set(smdata->tasklist, MUIA_List_First, v);

    __sprintf(smdata->tasklistinfobuf, (STRPTR)__(MSG_TASK_READY_AND_WAIT), smdata->sm_TasksReady, smdata->sm_TasksWaiting);
    set(smdata->tasklistinfo, MUIA_Text_Contents, smdata->tasklistinfobuf);
    set(smdata->tasklist, MUIA_List_Quiet, FALSE);
}

AROS_UFH3(struct TaskInfo *, TasksListConstructFunction,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(APTR, pool, A2),
    AROS_UFHA(struct Task *, curTask, A1))
{
    AROS_USERFUNC_INIT

    struct TaskInfo *ti = NULL;

    if ((ti = AllocVecPooled(pool, sizeof(struct TaskInfo))) != NULL)
    {
        ti->Task = curTask;

        /* Cache values we need incase something happens to the task .. */
        ti->TINode.ln_Type = curTask->tc_Node.ln_Type;
        ti->TINode.ln_Pri = (WORD)curTask->tc_Node.ln_Pri;
        switch (curTask->tc_State)
        {
        case TS_REMOVED:
            ti->TINode.ln_Name = StrDup("<tombstone>");
            break;
        case TS_RUN:
        case TS_READY:
        case TS_SPIN:
        case TS_WAIT:
            if (curTask->tc_Node.ln_Name)
            {
                ti->TINode.ln_Name = StrDup(curTask->tc_Node.ln_Name);
                break;
            }
        default:
            ti->TINode.ln_Name = StrDup("<unknown>");
            break;
        }

        AddTail(&((struct SysMonData *)h->h_Data)->sm_TaskList, &ti->TINode);
    }
    return ti;

    AROS_USERFUNC_EXIT
}

AROS_UFH3(VOID, TasksListDestructFunction,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(APTR, pool, A2),
    AROS_UFHA(struct TaskInfo *, ti, A1))
{
    AROS_USERFUNC_INIT
    
    Remove(&ti->TINode);
    FreeVec(ti->TINode.ln_Name);
    FreeVecPooled(pool, ti);
    
    AROS_USERFUNC_EXIT
}

AROS_UFH3(VOID, TaskSelectedFunction,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    struct SysMonData *smdata = h->h_Data;
    struct TaskInfo *ti = NULL;

    DoMethod(smdata->tasklist, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &ti);

    if (ti != NULL)
    {
        smdata->sm_TaskSelected = ti->Task;
    }
    else
        smdata->sm_TaskSelected = NULL;

    AROS_USERFUNC_EXIT
}


AROS_UFH3(VOID, TasksListDisplayFunction,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(STRPTR *, strings, A2),
    AROS_UFHA(struct TaskInfo *, ti, A1))
{
    AROS_USERFUNC_INIT

    static TEXT bufprio[8];

    if (ti)
    {
        __sprintf(bufprio, "%d", (LONG)ti->TINode.ln_Pri);

        strings[0] = ti->TINode.ln_Name;
        strings[1] = bufprio;
        strings[2] = ti->TINode.ln_Type == NT_TASK ? (STRPTR)_(MSG_TASK) : (STRPTR)_(MSG_PROCESS);
    }
    else
    {
        strings[0] = (STRPTR)_(MSG_TASK_NAME);
        strings[1] = (STRPTR)_(MSG_TASK_PRIORITY);
        strings[2] = (STRPTR)_(MSG_TASK_TYPE);
    }

    AROS_USERFUNC_EXIT
}

static BOOL InitTasks(struct SysMonData *smdata)
{
    TaskResBase = OpenResource("task.resource");
    if (TaskResBase == NULL)
    {
        FPuts(Output(), (STRPTR)_(MSG_CANT_OPEN_TASK_RESOURCE));
        return FALSE;
    }

    smdata->sm_Task = FindTask(NULL);
    NewList(&smdata->sm_TaskList);

    smdata->tasklistinfobuf = AllocVec(30, MEMF_PUBLIC);

    smdata->sm_TaskSelected = NULL;
    smdata->sm_TaskTotalRuntime = 0;

    return TRUE;
}

static VOID DeInitTasks(struct SysMonData *smdata)
{
    FreeVec(smdata->tasklistinfobuf);
}

struct SysMonModule tasksmodule =
{
    .Init = InitTasks,
    .DeInit = DeInitTasks,
};

