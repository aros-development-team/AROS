/*
    Copyright ©2010-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/config.h>

#define DEBUG 0
#include <aros/debug.h>

#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/task.h>
#include <proto/dos.h>

#include <exec/execbase.h>
#include <exec/rawfmt.h>

#include <libraries/mui.h>
#include <resources/task.h>

#include <string.h>

#include "locale.h"

#include "tasks.h"

#define DLIST(x)

/* uncomment the following line to force full-refresh */
//#define TASKLIST_FLUSHUPDATE
/* uncomment the following line to prevent updating */
//#define TASKLIST_NOTIMER

/* Task information handling*/
struct TaskInfo
{
    struct Node                 ti_Node;
    struct Task                 *ti_Task;
#if defined(__AROSPLATFORM_SMP__)
    cpuid_t                     ti_CPU;
#endif
    struct timeval              ti_TimeCurrent;
    struct timeval              ti_TimeLast;
    ULONG                       ti_CPUUsage;
    ULONG                       ti_Flags;
};

#define TIF_ENABLED             (1 << 0)
#define TIF_VALID               (1 << 1)

struct Tasklist_DATA
{
    APTR                        tld_taskresBase;

    struct MUI_EventHandlerNode tld_InputEvent;
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

    ULONG                       tld_TasksTotal;
    
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

#if defined(__AROSPLATFORM_SMP__)
    TEXT                        tld_BufCPU[5];
#endif
    TEXT                        tld_BufUsage[10];
    TEXT                        tld_BufTime[20];
    TEXT                        tld_BufName[100];
    TEXT                        tld_BufType[20];
    TEXT                        tld_BufPrio[20];
    TEXT                        tld_BufSortCol[20];
    ULONG                       tasklistSortColumn;
    ULONG                       tasklistSortMode;
    ULONG                       updateSpeed;
};

#define SETUP_TASKLIST_INST_DATA       struct Tasklist_DATA *data = INST_DATA(CLASS, self)
#define TaskResBase data->tld_taskresBase

CONST_STRPTR badstr_tmpl = " " MUIX_PH MUIX_B "%s";
CONST_STRPTR badval_tmpl = MUIX_PH MUIX_B "%d ";

static inline int refreshRetVal(int val, int a, int b)
{
    if ((val < 0) || (a == b))
        return -1;
    return 1;
}

int Tasklist__Refresh(struct Tasklist_DATA *data, struct TaskInfo *ti, int col)
{
    struct TagItem QueryTaskTags[] =
    {
        {TaskTag_CPUTime,       0       },
        {TaskTag_CPUUsage,      0       },
#if defined(__AROSPLATFORM_SMP__)
        {TaskTag_CPUNumber,     0       },
#endif
        {TAG_DONE,              0       }
    };
#if defined(__AROSPLATFORM_SMP__)
    IPTR cpuNum;
#endif
    int retVal = 0, startcol = 1;
    ULONG cpuusage = 0;

    /* Cache values we need incase something happens to the task .. */
    ti->ti_TimeLast.tv_secs = ti->ti_TimeCurrent.tv_secs;
    QueryTaskTags[0].ti_Data = (IPTR)&ti->ti_TimeCurrent;
    QueryTaskTags[1].ti_Data = (IPTR)&cpuusage;    
#if defined(__AROSPLATFORM_SMP__)
    QueryTaskTags[2].ti_Data = (IPTR)&cpuNum;
#endif

    if (ti->ti_Task)
    {
        switch (ti->ti_Task->tc_State)
        {
        case TS_RUN:
        case TS_READY:
        case TS_SPIN:
        case TS_WAIT:
            QueryTaskTagList(ti->ti_Task, QueryTaskTags);
            cpuusage = ((cpuusage >> 16) * 10000) >> 16;
#if defined(__AROSPLATFORM_SMP__)
            if (ti->ti_CPU != (cpuid_t)cpuNum)
                retVal = refreshRetVal(retVal, col, startcol);
            ti->ti_CPU = (cpuid_t)cpuNum;
            startcol++;
#endif

            if (ti->ti_CPUUsage != cpuusage)
                retVal = refreshRetVal(retVal, col, startcol);
            ti->ti_CPUUsage = cpuusage;
            startcol++;

            if (ti->ti_TimeLast.tv_secs != ti->ti_TimeCurrent.tv_secs)
                retVal = refreshRetVal(retVal, col, startcol);
            startcol++;
            
            if (ti->ti_Node.ln_Pri != ti->ti_Task->tc_Node.ln_Pri)
                retVal = refreshRetVal(retVal, col, startcol);
            ti->ti_Node.ln_Pri = ti->ti_Task->tc_Node.ln_Pri;
            startcol++;

            if (ti->ti_Node.ln_Type != ti->ti_Task->tc_Node.ln_Type)
                retVal = refreshRetVal(retVal, col, startcol);
            ti->ti_Node.ln_Type = ti->ti_Task->tc_Node.ln_Type;

            if (!(ti->ti_Flags & TIF_ENABLED))
                retVal = refreshRetVal(retVal, 1, 0);
            ti->ti_Flags |= TIF_ENABLED;

            break;

        default:
            if (!(ti->ti_Flags & TIF_ENABLED))
                retVal = refreshRetVal(retVal, 1, 0);
            ti->ti_Flags &= ~TIF_ENABLED;
            break;
        }
    }
    return retVal;
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

    DLIST(bug("[SysMon:TaskList] %s()\n", __func__));

    if ((ti = AllocVecPooled(pool, sizeof(struct TaskInfo))) != NULL)
    {
        ti->ti_Task = curTask;
        ti->ti_Flags = 0;

        Tasklist__Refresh(data, ti, data->tasklistSortColumn);

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

        data->tld_TasksTotal++;
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

    struct Tasklist_DATA *data = h->h_Data;

    DLIST(bug("[SysMon:TaskList] %s()\n", __func__));

    Remove(&ti->ti_Node);
    data->tld_TasksTotal--;

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
    DLIST(bug("[SysMon:TaskList] ti @  0x%p\n", ti);)

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

    DLIST(bug("[SysMon:TaskList] %s()\n", __func__));

    switch (data->tasklistSortColumn)
    {
#if defined(__AROSPLATFORM_SMP__)
#define COLUMNOFFSET 1
        case 1:
            if (!data->tasklistSortMode)
            {
                retval = (LONG)(ti2->ti_CPU - ti1->ti_CPU);
                if (retval == 0)
                {
                    retval = (LONG)(stricmp(ti1->ti_Node.ln_Name, ti2->ti_Node.ln_Name));
                }
            }
            else
            {
                retval = (LONG)(ti1->ti_CPU - ti2->ti_CPU);
                if (retval == 0)
                {
                    retval = (LONG)(stricmp(ti2->ti_Node.ln_Name, ti1->ti_Node.ln_Name));
                }
            }
            break;
#else
#define COLUMNOFFSET 0
#endif
        case (COLUMNOFFSET + 1):
            if (!data->tasklistSortMode)
            {
                retval = (LONG)(ti2->ti_CPUUsage - ti1->ti_CPUUsage);
                if (retval == 0)
                {
                    retval = (LONG)(ti2->ti_TimeCurrent.tv_usec - ti1->ti_TimeCurrent.tv_usec);
                    if (retval == 0)
                        retval = (LONG)(stricmp(ti1->ti_Node.ln_Name, ti2->ti_Node.ln_Name));
                }
            }
            else
            {
                retval = (LONG)(ti1->ti_CPUUsage - ti2->ti_CPUUsage);
                if (retval == 0)
                {
                    retval = (LONG)(ti1->ti_TimeCurrent.tv_usec - ti2->ti_TimeCurrent.tv_usec);
                    if (retval == 0)
                        retval = (LONG)(stricmp(ti2->ti_Node.ln_Name, ti1->ti_Node.ln_Name));
                }
            }
            break;


        case (COLUMNOFFSET + 2):
            if (!data->tasklistSortMode)
            {
                retval = (LONG)(ti2->ti_TimeCurrent.tv_secs - ti1->ti_TimeCurrent.tv_secs);
                if (retval == 0)
                {
                    retval = (LONG)(ti2->ti_TimeCurrent.tv_usec - ti1->ti_TimeCurrent.tv_usec);
                    if (retval == 0)
                        retval = (LONG)(stricmp(ti1->ti_Node.ln_Name, ti2->ti_Node.ln_Name));
                }
            }
            else
            {
                retval = (LONG)(ti1->ti_TimeCurrent.tv_secs - ti2->ti_TimeCurrent.tv_secs);
                if (retval == 0)
                {
                    retval = (LONG)(ti1->ti_TimeCurrent.tv_usec - ti2->ti_TimeCurrent.tv_usec);
                    if (retval == 0)
                        retval = (LONG)(stricmp(ti2->ti_Node.ln_Name, ti1->ti_Node.ln_Name));
                }
            }
            break;

        case (COLUMNOFFSET + 3):
            if (!data->tasklistSortMode)
            {
                retval = (LONG)(ti2->ti_Node.ln_Pri - ti1->ti_Node.ln_Pri);
                if (retval == 0)
                {
                    retval = (LONG)(stricmp(ti1->ti_Node.ln_Name, ti2->ti_Node.ln_Name));
                }
            }
            else
            {
                retval = (LONG)(ti1->ti_Node.ln_Pri - ti2->ti_Node.ln_Pri);
                if (retval == 0)
                {
                    retval = (LONG)(stricmp(ti2->ti_Node.ln_Name, ti1->ti_Node.ln_Name));
                }
            }
            break;
        case (COLUMNOFFSET + 4):
            if (!data->tasklistSortMode)
            {
                retval = (LONG)(ti1->ti_Node.ln_Type - ti2->ti_Node.ln_Type);
                if (retval == 0)
                {
                    retval = (LONG)(stricmp(ti1->ti_Node.ln_Name, ti2->ti_Node.ln_Name));
                }
            }
            else
            {
                retval = (LONG)(ti2->ti_Node.ln_Type - ti1->ti_Node.ln_Type);
                if (retval == 0)
                {
                    retval = (LONG)(stricmp(ti2->ti_Node.ln_Name, ti1->ti_Node.ln_Name));
                }
            }
            break;
        default:
            if (!data->tasklistSortMode)
                retval = (LONG)(stricmp(ti1->ti_Node.ln_Name, ti2->ti_Node.ln_Name));
            else
                retval = (LONG)(stricmp(ti2->ti_Node.ln_Name, ti1->ti_Node.ln_Name));
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
    IPTR fmtdata[] =
    {
        0,
        0,
        0,
        0,
        0
    };
    char *type;
    int col= 0;

    DLIST(bug("[SysMon:TaskList] %s()\n", __func__));

    if (ti)
    {
        type = (ti->ti_Node.ln_Type == NT_TASK) ? data->msg_task : data->msg_process;

        if (!(ti->ti_Flags & TIF_VALID))
        {
            fmtdata[0] = (IPTR)ti->ti_Node.ln_Name;
            RawDoFmt(badstr_tmpl, (RAWARG)&fmtdata, RAWFMTFUNC_STRING, data->tld_BufName);
            fmtdata[0] = (IPTR)ti->ti_Node.ln_Pri;
            RawDoFmt(badval_tmpl, (RAWARG)&fmtdata, RAWFMTFUNC_STRING, data->tld_BufPrio);
            fmtdata[0] = (IPTR)type;
            RawDoFmt(badstr_tmpl, (RAWARG)&fmtdata, RAWFMTFUNC_STRING, data->tld_BufType);
            strings[col++] = data->tld_BufName;
#if defined(__AROSPLATFORM_SMP__)
            strings[col++] = "---";
            strings[col++] = "---.--.--";
#endif
            strings[col++] = data->tld_BufPrio;
            strings[col++] = data->tld_BufType;
        }
        else
        {
            fmtdata[0] = (IPTR)ti->ti_Node.ln_Pri;
            RawDoFmt("%id ", (RAWARG)&fmtdata, RAWFMTFUNC_STRING, data->tld_BufPrio);
            strings[col++] = ti->ti_Node.ln_Name;
#if defined(__AROSPLATFORM_SMP__)
            fmtdata[0] = (IPTR)ti->ti_CPU;
            RawDoFmt("%03iu ", (RAWARG)&fmtdata, RAWFMTFUNC_STRING, data->tld_BufCPU);
            strings[col++] = data->tld_BufCPU;
#endif
            fmtdata[0] = (IPTR)ti->ti_CPUUsage / 100;
            fmtdata[1] = (IPTR)ti->ti_CPUUsage % 100;
            RawDoFmt("%3id.%02id%% ", (RAWARG)&fmtdata, RAWFMTFUNC_STRING, data->tld_BufUsage);
            strings[col++] = data->tld_BufUsage;

            fmtdata[0] = (IPTR)(ti->ti_TimeCurrent.tv_secs / 60 / 60);
            fmtdata[1] = (IPTR)((ti->ti_TimeCurrent.tv_secs / 60) % 60);
            fmtdata[2] = (IPTR)(ti->ti_TimeCurrent.tv_secs % 60);
            fmtdata[3] = (IPTR)((ti->ti_TimeCurrent.tv_usec + 500) / 10000);
            RawDoFmt("%3id:%02id:%02id.%02id", (RAWARG)&fmtdata, RAWFMTFUNC_STRING, data->tld_BufTime);
            strings[col++] = data->tld_BufTime;
            strings[col++] = data->tld_BufPrio;
            strings[col++] = type;
        }
    }
    else
    {
        char *dir;

        if (data->tasklistSortMode == 0)
            dir = "+";
        else
            dir = "-";

        fmtdata[1] = (IPTR)dir;
        if (data->tasklistSortColumn == col)
        {
            fmtdata[0] = (IPTR)data->msg_task_name;
            RawDoFmt(MUIX_B "%s %s", (RAWARG)&fmtdata, RAWFMTFUNC_STRING, data->tld_BufSortCol);
            strings[col++] = data->tld_BufSortCol;
        }
        else
            strings[col++] = data->msg_task_name;

#if defined(__AROSPLATFORM_SMP__)
        if (data->tasklistSortColumn == col)
        {
            fmtdata[0] = (IPTR)"CPU";
            RawDoFmt(MUIX_B "%s %s", (RAWARG)&fmtdata, RAWFMTFUNC_STRING, data->tld_BufSortCol);
            strings[col++] = data->tld_BufSortCol;
        }
        else
            strings[col++] = "CPU";
#endif

        if (data->tasklistSortColumn == col)
        {
            fmtdata[0] = (IPTR)"CPU %";
            RawDoFmt(MUIX_B "%s %s", (RAWARG)&fmtdata, RAWFMTFUNC_STRING, data->tld_BufSortCol);
            strings[col++] = data->tld_BufSortCol;
        }
        else
            strings[col++] = "CPU %";

        if (data->tasklistSortColumn == col)
        {
            fmtdata[0] = (IPTR)"CPU Time";
            RawDoFmt(MUIX_B "%s %s", (RAWARG)&fmtdata, RAWFMTFUNC_STRING, data->tld_BufSortCol);
            strings[col++] = data->tld_BufSortCol;
        }
        else
            strings[col++] = "CPU Time";

        if (data->tasklistSortColumn == col)
        {
            fmtdata[0] = (IPTR)data->msg_task_priority;
            RawDoFmt(MUIX_B "%s %s", (RAWARG)&fmtdata, RAWFMTFUNC_STRING, data->tld_BufSortCol);
            strings[col++] = data->tld_BufSortCol;
        }
        else
            strings[col++] = data->msg_task_priority;

        if (data->tasklistSortColumn == col)
        {
            fmtdata[0] = (IPTR)data->msg_task_type;
            RawDoFmt(MUIX_B "%s %s", (RAWARG)&fmtdata, RAWFMTFUNC_STRING, data->tld_BufSortCol);
            strings[col++] = data->tld_BufSortCol;
        }
        else
            strings[col++] = data->msg_task_type;
    }

    return NULL;

    AROS_USERFUNC_EXIT
}

Object *Tasklist__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    D(bug("[SysMon:TaskList] %s()\n", __func__));

    self = (Object *) DoSuperNewTags(CLASS, self, NULL,
        ReadListFrame,
        MUIA_List_Format,
#if !defined(__AROSPLATFORM_SMP__)
        "MIW=50 BAR,BAR,BAR,BAR P=\033r,",
#else
        "MIW=50 BAR,BAR,BAR,BAR,BAR P=\033r,",
#endif
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
            data->tasklistSortColumn = 1;

            data->msg_task = (STRPTR)_(MSG_TASK);
            data->msg_process = (STRPTR)_(MSG_PROCESS);
            data->msg_task_name = (STRPTR)_(MSG_TASK_NAME);
            data->msg_task_priority = (STRPTR)_(MSG_TASK_PRIORITY);
            data->msg_task_type = (STRPTR)_(MSG_TASK_TYPE);
            data->msg_task_tombstoned = (STRPTR)_(MSG_TASK_TOMBSTONE);
            data->msg_task_unknown = (STRPTR)_(MSG_TASK_UNKNOWN);

            data->tld_ConstructHook.h_Entry = (APTR)TasksListConstructFunction;
            data->tld_ConstructHook.h_Data = (APTR)data;
            data->tld_DestructHook.h_Entry = (APTR)TasksListDestructFunction;
            data->tld_DestructHook.h_Data = (APTR)data;
            data->tld_DisplayHook.h_Entry = (APTR)TasksListDisplayFunction;
            data->tld_DisplayHook.h_Data = (APTR)data;
            data->tld_CompareHook.h_Entry = (APTR)TaskCompareFunction;
            data->tld_CompareHook.h_Data = (APTR)data;

            SET(self, MUIA_List_ConstructHook, &data->tld_ConstructHook);
            SET(self, MUIA_List_DestructHook, &data->tld_DestructHook);
            SET(self, MUIA_List_DisplayHook, &data->tld_DisplayHook);
            SET(self, MUIA_List_CompareHook, &data->tld_CompareHook);

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
#ifndef TASKLIST_NOTIMER
            if (data->tld_TimerEvent.ihn_Method != 0)
            {
                DoMethod(_app(self), MUIM_Application_RemInputHandler, (IPTR) &data->tld_TimerEvent);
                data->tld_TimerEvent.ihn_Millis = data->updateSpeed;
                DoMethod(_app(self), MUIM_Application_AddInputHandler, (IPTR) &data->tld_TimerEvent);
            }
#endif
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
    IPTR retval = 0;

    D(bug("[SysMon:TaskList] %s()\n", __func__));

    switch (message->opg_AttrID)
    {
    case MUIA_Tasklist_Refreshed:
        *store = (IPTR)TRUE;
        retval = 1;
        break;
    case MUIA_Tasklist_RefreshMSecs:
        *store = (IPTR)data->updateSpeed;
        retval = 1;
        break;
    case MUIA_Tasklist_ReadyCount:
        *store = (IPTR)data->tld_TasksReady;
        retval = 1;
        break;
    case MUIA_Tasklist_WaitingCount:
        *store = (IPTR)data->tld_TasksWaiting;
        retval = 1;
        break;
    }

    if (!retval)
        retval = DoSuperMethodA(CLASS, self, (Msg) message);

    return retval;
}

IPTR Tasklist__MUIM_Show(Class *CLASS, Object *self, struct MUIP_Show *message)
{
    IPTR retval;

    SETUP_TASKLIST_INST_DATA;

    D(bug("[SysMon:TaskList] %s()\n", __func__));

    retval = DoSuperMethodA(CLASS, self, (Msg) message);

    data->tld_InputEvent.ehn_Events = IDCMP_MOUSEBUTTONS;
    data->tld_InputEvent.ehn_Priority = 10;
    data->tld_InputEvent.ehn_Flags = 0;
    data->tld_InputEvent.ehn_Object = self;
    data->tld_InputEvent.ehn_Class = CLASS;
    DoMethod(_win(self), MUIM_Window_AddEventHandler, (IPTR)&data->tld_InputEvent);

#ifndef TASKLIST_NOTIMER
    data->tld_TimerEvent.ihn_Flags  = MUIIHNF_TIMER;
    data->tld_TimerEvent.ihn_Millis = data->updateSpeed;
    data->tld_TimerEvent.ihn_Object = self;
    data->tld_TimerEvent.ihn_Method = MUIM_Tasklist_HandleTimer;

    DoMethod( _app(self), MUIM_Application_AddInputHandler, (IPTR) &data->tld_TimerEvent);
#endif

    DoMethod(self, MUIM_Tasklist_Refresh);

    return retval;
}

IPTR Tasklist__MUIM_Hide(Class *CLASS, Object *self, struct MUIP_Hide *message)
{
    SETUP_TASKLIST_INST_DATA;

    D(bug("[SysMon:TaskList] %s()\n", __func__));

#ifndef TASKLIST_NOTIMER
    DoMethod(_app(self), MUIM_Application_RemInputHandler, (IPTR) &data->tld_TimerEvent);
    data->tld_TimerEvent.ihn_Method = 0;
#endif

    DoMethod(_win(self), MUIM_Window_RemEventHandler, (IPTR)&data->tld_InputEvent);

    return DoSuperMethodA(CLASS, self, (Msg) message);
}

IPTR Tasklist__MUIM_HandleEvent(Class *CLASS, Object *self, struct MUIP_HandleEvent *message)
{
    SETUP_TASKLIST_INST_DATA;
    struct MUI_List_TestPos_Result selectres;

    D(bug("[SysMon:TaskList] %s()\n", __func__));

    if ((message->imsg->MouseX > _mleft(self)) &&
         (message->imsg->MouseY > _mtop(self)) &&
         (message->imsg->MouseX < _mright(self)) &&
         (message->imsg->MouseY < _mbottom(self)))
    {
        if ((message->imsg) && (message->imsg->Class == IDCMP_MOUSEBUTTONS))
        {
            if (message->imsg->Code == SELECTUP)
            {
                D(bug("[SysMon:TaskList] %s: Click @ %d, %d\n", __func__, message->imsg->MouseX, message->imsg->MouseY));
                DoMethod(self, MUIM_List_TestPos, message->imsg->MouseX, message->imsg->MouseY, &selectres);
                if ((selectres.entry == -1) && (selectres.column != -1) && (message->imsg->MouseY < (_mtop(self) + _font(self)->tf_YSize + 4)))
                {
                    if (data->tasklistSortColumn == selectres.column)
                        data->tasklistSortMode = ~data->tasklistSortMode;
                    else
                        data->tasklistSortMode = 0;
                    data->tasklistSortColumn = selectres.column;
                    SET(self, MUIA_List_Quiet, TRUE);
                    DoMethod(self, MUIM_List_Sort);
                    DoMethod(self, MUIM_List_Redraw, MUIV_List_Redraw_All);
                    SET(self, MUIA_List_Quiet, FALSE);
                }
            }
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
    BOOL doSort = FALSE;

    SET(self, MUIA_List_Quiet, TRUE);

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
                int taskUpdate;

                D(bug("[SysMon:TaskList] updating entry @ 0x%p\n", ti));

                if ((taskUpdate = Tasklist__Refresh(data, ti, data->tasklistSortColumn)) != 0)
                {
                    DoMethod(self, MUIM_List_Redraw, MUIV_List_Redraw_Entry, ti);
                    if (taskUpdate < 0)
                        doSort = TRUE;
                }

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
                SET(self, MUIA_List_Active, entryid);
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

    SET(self, MUIA_List_First, v);
#endif

    if (doSort)
        DoMethod(self, MUIM_List_Sort);

    SET(self, MUIA_List_Quiet, FALSE);
    SET(self, MUIA_Tasklist_Refreshed, TRUE);

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
