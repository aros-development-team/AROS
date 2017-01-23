/*
    Copyright 2010-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/task.h>
#include <proto/dos.h>

#include <clib/alib_protos.h>

#include <exec/execbase.h>
#include <libraries/mui.h>
#include <resources/task.h>

#include "locale.h"

#include "tasks.h"

//#define TASKLIST_FLUSHUPDATE

/* Task information handling*/
struct TaskInfo
{
    struct Node                 ti_Node;
    struct Task                 *ti_Task;
    ULONG                       ti_TimeCurrent;
    ULONG                       ti_TimeLast;
    ULONG                       ti_Flags;
};

#define TIF_ENABLED             (1 << 0)
#define TIF_VALID               (1 << 1)

struct Tasklist_DATA
{
    APTR                        tld_taskresBase;

    struct MUI_InputHandlerNode tld_TimerEvent;

    struct Hook                 tld_ConstructHook;
    struct Hook                 tld_DestructHook;
    struct Hook                 tld_DisplayHook;
    struct Hook                 tld_CompareHook;

#ifdef TASKLIST_FLUSHUPDATE
    struct Hook                 tld_SelectedHook;
    struct Task                 *tld_TaskSelected;
#endif
    struct List                 tld_TaskList;

    ULONG                       tld_TasksWaiting;
    ULONG                       tld_TasksReady;
    ULONG                       tld_TaskTotalRuntime;
    
    STRPTR                      msg_task;
    STRPTR                      msg_process;
    STRPTR                      msg_task_name;
    STRPTR                      msg_task_priority;
    STRPTR                      msg_task_type;
    STRPTR                      msg_task_tombstoned;
    STRPTR                      msg_task_unknown;

    TEXT                        tld_BufName[100];
    TEXT                        tld_BufType[20];
    TEXT                        tld_BufPrio[20];

    ULONG                       tasklistSortColumn;
    ULONG                       updateSpeed;
};

#define SETUP_TASKLIST_INST_DATA       struct Tasklist_DATA *data = INST_DATA(CLASS, self)
#define TaskResBase data->tld_taskresBase

CONST_STRPTR badstr_tmpl = MUIX_PH MUIX_B "%s";
CONST_STRPTR badval_tmpl = MUIX_PH MUIX_B "%d";

VOID RefreshTask(struct TaskInfo *ti)
{
    /* Cache values we need incase something happens to the task .. */
    ti->ti_Node.ln_Type = ti->ti_Task->tc_Node.ln_Type;
    ti->ti_Node.ln_Pri = ti->ti_Task->tc_Node.ln_Pri;
    ti->ti_Flags |= TIF_ENABLED;
}

#ifndef TASKLIST_FLUSHUPDATE
IPTR Tasklist__DeleteTaskEntry(Class *CLASS, Object *self, struct TaskInfo *ti)
{
    int i;

    for (i=0;;i++)
    {
        struct TaskInfo *le_ti = NULL;

        DoMethod(self, MUIM_List_GetEntry, i, &le_ti);
        if (!le_ti) break;

        if (ti == le_ti)
        {
            D(bug("[SysMon:TaskList] deleting entry ...\n"));
            DoMethod(self, MUIM_List_Remove, i);
            break;
        }
    }
    return (IPTR)NULL;
}
#endif

AROS_UFH3(struct TaskInfo *, TasksListConstructFunction,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(APTR, pool, A2),
    AROS_UFHA(struct Task *, curTask, A1))
{
    AROS_USERFUNC_INIT

    struct Tasklist_DATA *data = h->h_Data;
    struct TaskInfo *ti = NULL;

    D(bug("[SysMon:TaskList] %s()\n", __func__));

    if ((ti = AllocVecPooled(pool, sizeof(struct TaskInfo))) != NULL)
    {
        ti->ti_Task = curTask;
        ti->ti_Flags = 0;

        RefreshTask(ti);
        ti->ti_Flags &= ~TIF_VALID;

        switch (curTask->tc_State)
        {
        case TS_REMOVED:
            ti->ti_Node.ln_Name = StrDup(data->msg_task_tombstoned);
            break;
        case TS_RUN:
        case TS_READY:
        case TS_SPIN:
        case TS_WAIT:
            ti->ti_Flags |= TIF_VALID;
            if (curTask->tc_Node.ln_Name)
            {
                ti->ti_Node.ln_Name = StrDup(curTask->tc_Node.ln_Name);
                break;
            }
        default:
            ti->ti_Node.ln_Name = StrDup(data->msg_task_unknown);
            break;
        }

        AddTail(&data->tld_TaskList, &ti->ti_Node);
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

    D(bug("[SysMon:TaskList] %s()\n", __func__));

    Remove(&ti->ti_Node);
    FreeVec(ti->ti_Node.ln_Name);
    FreeVecPooled(pool, ti);
    
    AROS_USERFUNC_EXIT
}

#ifdef TASKLIST_FLUSHUPDATE
AROS_UFH3(VOID, TaskSelectedFunction,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    struct Tasklist_DATA *data = h->h_Data;
    struct TaskInfo *ti = NULL;

    DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &ti);
    bug("[SysMon:TaskList] ti @  0x%p\n", ti);

    if (ti != NULL)
    {
        data->tld_TaskSelected = ti->ti_Task;
    }
    else
        data->tld_TaskSelected = NULL;

    AROS_USERFUNC_EXIT
}
#endif

AROS_UFH3(LONG, TaskCompareFunction,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(struct TaskInfo *, ti2, A2),
    AROS_UFHA(struct TaskInfo *, ti1, A1))
{
    AROS_USERFUNC_INIT

    struct Tasklist_DATA *data = h->h_Data;
    LONG retval;

    D(bug("[SysMon:TaskList] %s()\n", __func__));

    switch (data->tasklistSortColumn)
    {
        case 1:
            retval = (LONG)(ti1->ti_Node.ln_Pri - ti2->ti_Node.ln_Pri);
            break;
        case 2:
            retval = (LONG)(ti1->ti_Node.ln_Type - ti2->ti_Node.ln_Type);
            break;
        default:
            retval = (LONG)(stricmp(ti1->ti_Node.ln_Name, ti2->ti_Node.ln_Name));
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

    struct Tasklist_DATA *data = h->h_Data;
    char *type;

    D(bug("[SysMon:TaskList] %s()\n", __func__));

    if (ti)
    {
        type = (ti->ti_Node.ln_Type == NT_TASK) ? data->msg_task : data->msg_process;

        if (!(ti->ti_Flags & TIF_VALID))
        {
            __sprintf(data->tld_BufName, badstr_tmpl, ti->ti_Node.ln_Name);
            __sprintf(data->tld_BufPrio, badval_tmpl, ti->ti_Node.ln_Pri);
            __sprintf(data->tld_BufType, badstr_tmpl, type);
            strings[0] = data->tld_BufName;
            strings[2] = data->tld_BufType;
        }
        else
        {
            __sprintf(data->tld_BufPrio, "%d", ti->ti_Node.ln_Pri);
            strings[0] = ti->ti_Node.ln_Name;
            strings[2] = type;
        }

        strings[1] = data->tld_BufPrio;
    }
    else
    {
        strings[0] = data->msg_task_name;
        strings[1] = data->msg_task_priority;
        strings[2] = data->msg_task_type;
    }

    return NULL;

    AROS_USERFUNC_EXIT
}

Object *Tasklist__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    D(bug("[SysMon:TaskList] %s()\n", __func__));

    self = (Object *) DoSuperNewTags(CLASS, self, NULL,
        ReadListFrame,
        MUIA_List_Format, "MIW=50 BAR,BAR P=\033r,",
        MUIA_List_Title, (IPTR)TRUE,
        TAG_MORE, (IPTR) message->ops_AttrList
    );

    if (self != NULL)
    {
        SETUP_TASKLIST_INST_DATA;

        data->tld_taskresBase = OpenResource("task.resource");
        if (data->tld_taskresBase)
        {
            NewList(&data->tld_TaskList);

            data->updateSpeed = (ULONG)GetTagData(MUIA_Tasklist_RefreshMSecs, MUIV_Tasklist_Refresh_Normal, message->ops_AttrList);

            data->tld_TaskTotalRuntime = 0;

            data->msg_task = (STRPTR)_(MSG_TASK);
            data->msg_process = (STRPTR)_(MSG_PROCESS);
            data->msg_task_name = (STRPTR)_(MSG_TASK_NAME);
            data->msg_task_priority = (STRPTR)_(MSG_TASK_PRIORITY);
            data->msg_task_type = (STRPTR)_(MSG_TASK_TYPE);
            data->msg_task_tombstoned = (STRPTR)"<tombstone>";
            data->msg_task_unknown = (STRPTR)"<unknown>";

            data->tld_ConstructHook.h_Entry = (APTR)TasksListConstructFunction;
            data->tld_ConstructHook.h_Data = (APTR)data;
            data->tld_DestructHook.h_Entry = (APTR)TasksListDestructFunction;
            data->tld_DestructHook.h_Data = (APTR)data;
            data->tld_DisplayHook.h_Entry = (APTR)TasksListDisplayFunction;
            data->tld_DisplayHook.h_Data = (APTR)data;
            data->tld_CompareHook.h_Entry = (APTR)TaskCompareFunction;
            data->tld_CompareHook.h_Data = (APTR)data;

            set(self, MUIA_List_ConstructHook, &data->tld_ConstructHook);
            set(self, MUIA_List_DestructHook, &data->tld_DestructHook);
            set(self, MUIA_List_DisplayHook, &data->tld_DisplayHook);
            set(self, MUIA_List_CompareHook, &data->tld_CompareHook);

#ifdef TASKLIST_FLUSHUPDATE
            data->tld_TaskSelected = NULL;
            data->tld_SelectedHook.h_Entry = (APTR)TaskSelectedFunction;
            data->tld_SelectedHook.h_Data = (APTR)data;
            DoMethod(self, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
                self, 2, MUIM_CallHook, (IPTR)&data->tld_SelectedHook);
#endif
        }
        else
        {
            FPuts(Output(), (STRPTR)_(MSG_CANT_OPEN_TASK_RESOURCE));

            CoerceMethod(CLASS, self, OM_DISPOSE);

            self = NULL;
        }
    }

    return self;
}

IPTR Tasklist__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    D(bug("[SysMon:TaskList] %s()\n", __func__));

    return DoSuperMethodA(CLASS, self, message);
}

IPTR Tasklist__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_TASKLIST_INST_DATA;
    struct TagItem  *tstate = message->ops_AttrList, *tag;

    D(bug("[SysMon:TaskList] %s()\n", __func__));

    while ((tag = NextTagItem((struct TagItem **)&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Tasklist_RefreshMSecs:
            data->updateSpeed = (ULONG)tag->ti_Data;
            if (data->tld_TimerEvent.ihn_Method != 0)
            {
                DoMethod(_app(self), MUIM_Application_RemInputHandler, (IPTR) &data->tld_TimerEvent);
                data->tld_TimerEvent.ihn_Millis = data->updateSpeed;
                DoMethod(_app(self), MUIM_Application_AddInputHandler, (IPTR) &data->tld_TimerEvent);
            }
            break;
        case MUIA_Tasklist_Refreshed:
            break;
        }
    }
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

IPTR Tasklist__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    SETUP_TASKLIST_INST_DATA;
    IPTR *store = message->opg_Storage;

    D(bug("[SysMon:TaskList] %s()\n", __func__));

    switch (message->opg_AttrID)
    {
    case MUIA_Tasklist_Refreshed:
        *store = (IPTR)TRUE;
        break;
    case MUIA_Tasklist_RefreshMSecs:
        *store = (IPTR)data->updateSpeed;
        break;
    case MUIA_Tasklist_ReadyCount:
        *store = (IPTR)data->tld_TasksReady;
        break;
    case MUIA_Tasklist_WaitingCount:
        *store = (IPTR)data->tld_TasksWaiting;
        break;
    }

    return DoSuperMethodA(CLASS, self, (Msg) message);
}

IPTR Tasklist__MUIM_Show(Class *CLASS, Object *self, struct MUIP_Show *message)
{
    IPTR retval;

    SETUP_TASKLIST_INST_DATA;

    D(bug("[SysMon:TaskList] %s()\n", __func__));

    retval = DoSuperMethodA(CLASS, self, (Msg) message);

    data->tld_TimerEvent.ihn_Flags  = MUIIHNF_TIMER;
    data->tld_TimerEvent.ihn_Millis = data->updateSpeed;
    data->tld_TimerEvent.ihn_Object = self;
    data->tld_TimerEvent.ihn_Method = MUIM_Tasklist_HandleTimer;

    DoMethod( _app(self), MUIM_Application_AddInputHandler, (IPTR) &data->tld_TimerEvent);

    return retval;
}

IPTR Tasklist__MUIM_Hide(Class *CLASS, Object *self, struct MUIP_Hide *message)
{
    SETUP_TASKLIST_INST_DATA;

    D(bug("[SysMon:TaskList] %s()\n", __func__));
    
    DoMethod(_app(self), MUIM_Application_RemInputHandler, (IPTR) &data->tld_TimerEvent);
    data->tld_TimerEvent.ihn_Method = 0;
    
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

IPTR Tasklist__MUIM_HandleEvent(Class *CLASS, Object *self, struct MUIP_HandleEvent *message)
{
    SETUP_TASKLIST_INST_DATA;
    struct MUI_List_TestPos_Result selectres;

    bug("[SysMon:TaskList] %s()\n", __func__);

    if ((message->imsg) && (message->imsg->Class == IDCMP_MOUSEBUTTONS))
    {
        if (message->imsg->Code == SELECTUP)
        {
            // TODO: check if we have been clicked on the column header and set
            // sorting appropriately ...
            bug("[SysMon:TaskList] %s: Click @ %d, %d\n", __func__, message->imsg->MouseX, message->imsg->MouseY);
            DoMethod(self, MUIM_List_TestPos, message->imsg->MouseX, message->imsg->MouseY, &selectres);
            bug("[SysMon:TaskList] %s: pos entry #%d\n", __func__, selectres.entry);
            bug("[SysMon:TaskList] %s: pos column #%d\n", __func__, selectres.column);
            bug("[SysMon:TaskList] %s: pos x,y = %d,%d\n", __func__, selectres.xoffset, selectres.yoffset);

            data->tasklistSortColumn = 0;
        }
    }

    return DoSuperMethodA(CLASS, self, (Msg) message);
}

IPTR Tasklist__MUIM_Tasklist_Refresh(Class *CLASS, Object *self, Msg message)
{
    SETUP_TASKLIST_INST_DATA;

    D(bug("[SysMon:TaskList] %s()\n", __func__));

    struct TaskList *systasklist;
    struct Task * task;
#ifndef TASKLIST_FLUSHUPDATE
    struct TaskInfo *ti = NULL, *titmp;
#else
    struct Task *selected = data->tld_TaskSelected;
    IPTR firstvis = 0;
    IPTR entryid = 0;
#endif

    set(self, MUIA_List_Quiet, TRUE);

#ifdef TASKLIST_FLUSHUPDATE
    get(self, MUIA_List_First, &firstvis);
    DoMethod(self, MUIM_List_Clear);
#else
    ForeachNode(&data->tld_TaskList, ti)
    {
        ti->ti_Flags &= ~TIF_ENABLED;
    }
#endif

    data->tld_TasksWaiting = 0;
    data->tld_TasksReady = 0;
    data->tld_TaskTotalRuntime = 0;

    systasklist = LockTaskList(LTF_ALL);
    while ((task = NextTaskEntry(systasklist, LTF_ALL)) != NULL)
    {
        D(bug("[SysMon:TaskList] task %s state %d\n", task->tc_Node.ln_Name, task->tc_State));
        
        if (task->tc_State == TS_READY)
        {
            data->tld_TasksReady++;
        }
        if ((task->tc_State == TS_WAIT) || (task->tc_State == TS_SPIN))
        {
            data->tld_TasksWaiting++;
        }
       
#ifndef TASKLIST_FLUSHUPDATE
        ti = NULL;
        ForeachNode(&data->tld_TaskList, ti)
        {
            if (ti->ti_Task == task)
            {
                D(bug("[SysMon:TaskList] updating entry @ 0x%p\n", ti));
                RefreshTask(ti);
                task = NULL;
                break;
            }
        }
#endif
        if (task)
        {
            D(bug("[SysMon:TaskList] creating new entry ...\n"));
#ifdef TASKLIST_FLUSHUPDATE
            entryid =
#endif
                DoMethod(self, MUIM_List_InsertSingle, task, MUIV_List_Insert_Sorted);
#ifdef TASKLIST_FLUSHUPDATE
            if (task == selected)
            {
                set(self, MUIA_List_Active, entryid);
            }
#endif
        }
    }
    UnLockTaskList(systasklist, LTF_ALL);

#ifndef TASKLIST_FLUSHUPDATE
    ti = NULL;
    ForeachNodeSafe(&data->tld_TaskList, ti, titmp)
    {
        if (!(ti->ti_Flags & TIF_ENABLED))
        {
            Tasklist__DeleteTaskEntry(CLASS, self, ti);
        }
    }
#else
    if (XGET(self, MUIA_List_Active) == 0)
        data->tld_TaskSelected = NULL;

    /* Keep the "pre-refresh" view on List as much as possible */
    LONG v = (LONG)firstvis;
    if (firstvis + XGET(self, MUIA_List_Visible) >= XGET(self, MUIA_List_Entries))
    {
        v = XGET(self, MUIA_List_Entries) - XGET(self, MUIA_List_Visible);
        if (v < 0) v = 0;
    }

    set(self, MUIA_List_First, v);
#endif

    set(self, MUIA_List_Quiet, FALSE);
    set(self, MUIA_Tasklist_Refreshed, TRUE);

    return (IPTR)TRUE;
}

IPTR Tasklist__MUIM_Tasklist_HandleTimer(Class *CLASS, Object *self, Msg message)
{
    D(bug("[SysMon:TaskList] %s()\n", __func__));

    DoMethod(self, MUIM_Tasklist_Refresh);

    return (IPTR)FALSE;
}

/*** Setup ******************************************************************/
TASKLIST_CUSTOMCLASS
(
  Tasklist, NULL, MUIC_List, NULL,
  OM_NEW,                             struct opSet *,
  OM_DISPOSE,                         Msg,
  OM_SET,                             struct opSet *,
  OM_GET,                             struct opGet *,
  MUIM_Show,                          struct MUIP_Show *,
  MUIM_Hide,                          struct MUIP_Hide *,
  MUIM_HandleEvent,                   struct MUIP_HandleEvent *,
  MUIM_Tasklist_Refresh,              Msg,
  MUIM_Tasklist_HandleTimer,          Msg
);
