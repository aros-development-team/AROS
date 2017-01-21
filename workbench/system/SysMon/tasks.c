/*
    Copyright 2010-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

//#define DEBUG 1
#include <aros/debug.h>

#include <exec/execbase.h>
#include <resources/task.h>
#include <clib/alib_protos.h>

#include <proto/task.h>
#include <proto/dos.h>

#include "locale.h"

#include "sysmon_intern.h"

//#define TASKLIST_FLUSHUPDATE

APTR TaskResBase = NULL;

/* Task information handling*/
struct TaskInfo
{
    struct Node TINode;
    struct Task *Task;
    ULONG       TimeCurrent;
    ULONG       TimeLast;
    ULONG       Flags;
};

#define TIF_ENABLED             (1 << 0)
#define TIF_VALID               (1 << 1)

VOID RefreshTask(struct TaskInfo *ti)
{
    /* Cache values we need incase something happens to the task .. */
    ti->TINode.ln_Type = ti->Task->tc_Node.ln_Type;
    ti->TINode.ln_Pri = ti->Task->tc_Node.ln_Pri;
    ti->Flags |= TIF_ENABLED;
}

#ifndef TASKLIST_FLUSHUPDATE
VOID DeleteTaskEntry(struct SysMonData *smdata, struct TaskInfo *ti)
{
    int i;

    for (i=0;;i++)
    {
        struct TaskInfo *le_ti = NULL;

        DoMethod(smdata->tasklist, MUIM_List_GetEntry, i, &le_ti);
        if (!le_ti) break;

        if (ti == le_ti)
        {
            D(bug("[SysMon] deleting entry ...\n"));
            DoMethod(smdata->tasklist, MUIM_List_Remove, i);
            break;
        }
    }
}
#endif

VOID UpdateTasksInformation(struct SysMonData *smdata)
{
    struct TaskList *systasklist;
    struct Task * task;
#ifndef TASKLIST_FLUSHUPDATE
    struct TaskInfo *ti = NULL, *titmp;
#else
    struct Task *selected = smdata->sm_TaskSelected;
    IPTR firstvis = 0;
    IPTR entryid = 0;
#endif

    set(smdata->tasklist, MUIA_List_Quiet, TRUE);

#ifdef TASKLIST_FLUSHUPDATE
    get(smdata->tasklist, MUIA_List_First, &firstvis);
    DoMethod(smdata->tasklist, MUIM_List_Clear);
#else
    ForeachNode(&smdata->sm_TaskList, ti)
    {
        ti->Flags &= ~TIF_ENABLED;
    }
#endif

    smdata->sm_TasksWaiting = 0;
    smdata->sm_TasksReady = 0;
    smdata->sm_TaskTotalRuntime = 0;

    systasklist = LockTaskList(LTF_ALL);
    while ((task = NextTaskEntry(systasklist, LTF_ALL)) != NULL)
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
       
#ifndef TASKLIST_FLUSHUPDATE
        ti = NULL;
        ForeachNode(&smdata->sm_TaskList, ti)
        {
            if (ti->Task == task)
            {
                D(bug("[SysMon] updating entry @ 0x%p\n", ti));
                RefreshTask(ti);
                task = NULL;
                break;
            }
        }
#endif
        if (task)
        {
            D(bug("[SysMon] creating new entry ...\n"));
#ifdef TASKLIST_FLUSHUPDATE
            entryid =
#endif
                DoMethod(smdata->tasklist, MUIM_List_InsertSingle, task, MUIV_List_Insert_Sorted);
#ifdef TASKLIST_FLUSHUPDATE
            if (task == selected)
            {
                set(smdata->tasklist, MUIA_List_Active, entryid);
            }
#endif
        }
    }
    UnLockTaskList(systasklist, LTF_ALL);

#ifndef TASKLIST_FLUSHUPDATE
    ti = NULL;
    ForeachNodeSafe(&smdata->sm_TaskList, ti, titmp)
    {
        if (!(ti->Flags & TIF_ENABLED))
        {
            DeleteTaskEntry(smdata, ti);
        }
    }
#else
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
#endif

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
        ti->Flags = 0;

        RefreshTask(ti);
        ti->Flags &= ~TIF_VALID;

        switch (curTask->tc_State)
        {
        case TS_REMOVED:
            ti->TINode.ln_Name = StrDup("<tombstone>");
            break;
        case TS_RUN:
        case TS_READY:
        case TS_SPIN:
        case TS_WAIT:
            ti->Flags |= TIF_VALID;
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

 
AROS_UFH3(LONG, TaskCompareFunction,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(struct TaskInfo *, ti2, A2),
    AROS_UFHA(struct TaskInfo *, ti1, A1))
{
    AROS_USERFUNC_INIT

    struct SysMonData *smdata = h->h_Data;
    LONG retval;

    switch (smdata->tasklistSortColumn)
    {
        case 1:
            retval = (LONG)(ti1->TINode.ln_Pri - ti2->TINode.ln_Pri);
            break;
        case 2:
            retval = (LONG)(ti1->TINode.ln_Type - ti2->TINode.ln_Type);
            break;
        default:
            retval = (LONG)(stricmp(ti1->TINode.ln_Name, ti2->TINode.ln_Name));
            break;
    }

    return retval;

    AROS_USERFUNC_EXIT
}


AROS_UFH3(APTR, TasksListDisplayFunction,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(STRPTR *, strings, A2),
    AROS_UFHA(struct TaskInfo *, ti, A1))
{
    AROS_USERFUNC_INIT

    struct SysMonData *smdata = h->h_Data;
    char *type;

    if (ti)
    {
        type = ti->TINode.ln_Type == NT_TASK ? (STRPTR)_(MSG_TASK) : (STRPTR)_(MSG_PROCESS);

        if (!(ti->Flags & TIF_VALID))
        {
            __sprintf(smdata->bufname, MUIX_PH MUIX_B "%s", ti->TINode.ln_Name);
            __sprintf(smdata->bufprio, MUIX_PH MUIX_B "%d", (LONG)ti->TINode.ln_Pri);
            __sprintf(smdata->buftype, MUIX_PH MUIX_B "%s", type);
            strings[0] = smdata->bufname;
            strings[2] = smdata->buftype;
        }
        else
        {
            __sprintf(smdata->bufprio, "%d", (LONG)ti->TINode.ln_Pri);
            strings[0] = ti->TINode.ln_Name;
            strings[2] = type;
        }

        strings[1] = smdata->bufprio;
    }
    else
    {
        strings[0] = (STRPTR)_(MSG_TASK_NAME);
        strings[1] = (STRPTR)_(MSG_TASK_PRIORITY);
        strings[2] = (STRPTR)_(MSG_TASK_TYPE);
    }

    return NULL;

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

