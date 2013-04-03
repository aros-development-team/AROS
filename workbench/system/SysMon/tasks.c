/*
    Copyright 2010-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/execbase.h>
#include <clib/alib_protos.h>

#include "locale.h"

#include "sysmon_intern.h"

/* Task information handling*/
struct TaskInfo
{
    struct Node TINode; /* We copy the tasks info into our node, with ln_Name pointing to TaskInfo->Private */
    struct Task *Task;
    ULONG TimeCurrent;
    ULONG TimeLast;
    UBYTE Private; /* MUST ALWAYS BE LAST. HERE NAME WILL BE COPIED */
};

VOID UpdateTasksInformation(struct SysMonData * smdata)
{
    struct Task * task;
    IPTR firstvis = 0, entryid = 0;
    struct Task *selected = smdata->sm_TaskSelected;

    set(smdata->tasklist, MUIA_List_Quiet, TRUE);

    get(smdata->tasklist, MUIA_List_First, &firstvis);

    DoMethod(smdata->tasklist, MUIM_List_Clear);

    smdata->sm_TasksWaiting = 0;
    smdata->sm_TasksReady = 0;
    smdata->sm_TaskTotalRuntime = 0;

    /* We are unlikely to dissapear and this code still run .. so dont disable yet */
    if ((entryid = DoMethod(smdata->tasklist, MUIM_List_InsertSingle, smdata->sm_Task, MUIV_List_Insert_Bottom)) == 0)
    {
        return FALSE;
    }
    if (smdata->sm_Task == selected)
    {
        set(smdata->tasklist, MUIA_List_Active, entryid);
    }

    /* Now disable multitasking and get the rest of the tasks .. */
    Disable();
    for (task = (struct Task *)SysBase->TaskReady.lh_Head;
        task->tc_Node.ln_Succ != NULL;
        task = (struct Task *)task->tc_Node.ln_Succ)
    {
        if ((entryid = DoMethod(smdata->tasklist, MUIM_List_InsertSingle, task, MUIV_List_Insert_Bottom)) == 0)
        {
            Enable();
            return FALSE;
        }
        if (task == selected)
        {
            set(smdata->tasklist, MUIA_List_Active, entryid);
        }

        smdata->sm_TasksReady++;
    }

    for (task = (struct Task *)SysBase->TaskWait.lh_Head;
        task->tc_Node.ln_Succ != NULL;
        task = (struct Task *)task->tc_Node.ln_Succ)
    {
        if ((entryid = DoMethod(smdata->tasklist, MUIM_List_InsertSingle, task, MUIV_List_Insert_Bottom)) == 0)
        {
            Enable();
            return FALSE;
        }
        if (task == selected)
        {
            set(smdata->tasklist, MUIA_List_Active, entryid);
        }

        smdata->sm_TasksWaiting++;
    }

    Enable();

    if (XGET(smdata->tasklist, MUIA_List_Active) == 0)
        smdata->sm_TaskSelected = NULL;

    if (firstvis < XGET(smdata->tasklist, MUIA_List_Entries))
    {
        if ((XGET(smdata->tasklist, MUIA_List_Entries) - firstvis) > XGET(smdata->tasklist, MUIA_List_Visible))
            set(smdata->tasklist, MUIA_List_First, firstvis);
        else
            set(smdata->tasklist, MUIA_List_First, (XGET(smdata->tasklist, MUIA_List_Entries) - XGET(smdata->tasklist, MUIA_List_Visible)));
    }
    __sprintf(smdata->tasklistinfobuf, "%ld ready, %ld waiting", smdata->sm_TasksReady, smdata->sm_TasksWaiting);
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
        ti->TINode.ln_Name = StrDup(curTask->tc_Node.ln_Name);

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
    smdata->sm_Task = FindTask(NULL);
    NewList(&smdata->sm_TaskList);

    smdata->tasklistinfobuf = AllocVec(30, MEMF_PUBLIC);

    smdata->sm_TaskSelected = NULL;
    smdata->sm_TaskTotalRuntime = 0;

    return TRUE;
}

static VOID DeInitTasks(struct SysMonData *smdata)
{
}

struct SysMonModule tasksmodule =
{
    .Init = InitTasks,
    .DeInit = DeInitTasks,
};

