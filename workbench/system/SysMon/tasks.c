
#include <exec/execbase.h>
#include <clib/alib_protos.h>

#include "locale.h"

/* Task information handling*/
struct TaskInfo
{
    STRPTR Name;
    WORD Type;
    WORD Priority;
    struct TaskInfo * Next;
    UBYTE Private; /* MUST ALWAYS BE LAST. HERE NAME WILL BE COPIED */
};

UBYTE taskinfobuffer[16 * 1024]; /* 16 kB buffer */

static LONG AddTaskInfo(struct Task * task, struct TaskInfo * ti)
{
    ti->Type = task->tc_Node.ln_Type;
    ti->Priority = (WORD)task->tc_Node.ln_Pri;
    ti->Name = (STRPTR)&ti->Private;
    ULONG namesize = 0;
    STRPTR src = task->tc_Node.ln_Name;

    if (src)
    {
        while(*src != 0)
        {
            ti->Name[namesize++] = *src;
            src++;
        }
    }

    *(ti->Name + namesize) = 0; /* Terminate */

    /* Calculate next item  */
    ti->Next = (struct TaskInfo *)(((UBYTE *)ti) + sizeof(struct TaskInfo) + namesize);

    return 1;
}

static BOOL FillTaskInfoBuffer(UBYTE * buffer)
{
    struct TaskInfo * current = (struct TaskInfo *)buffer;
    struct Task * task;

    Disable();
    if(!AddTaskInfo(SysBase->ThisTask, current))
    {
        Enable();
        return FALSE;
    }

    for(task=(struct Task *)SysBase->TaskReady.lh_Head;
        task->tc_Node.ln_Succ!=NULL;
        task=(struct Task *)task->tc_Node.ln_Succ)
    {
        current = current->Next;

        if(!AddTaskInfo(task, current))
        {
            Enable();
            return FALSE;
        }
    }

    for(task=(struct Task *)SysBase->TaskWait.lh_Head;
        task->tc_Node.ln_Succ!=NULL;
        task=(struct Task *)task->tc_Node.ln_Succ)
    {
        current = current->Next;

        if(!AddTaskInfo(task, current))
        {
            Enable();
            return FALSE;
        }
    }
    
    current->Next = NULL; /* "stop" list */

    Enable();
    return TRUE;
}

VOID UpdateTasksInformation(struct SysMonData * smdata)
{
    ULONG firstvis;

    set(smdata->tasklist, MUIA_List_Quiet, TRUE);

    get(smdata->tasklist, MUIA_List_First, &firstvis);
    
    /* Clear prior to reading information, because list contains items
    from the taskinfobuffer. Once FillTaskInfoBuffer is executed old items
    are invalid and could crash list */     
    DoMethod(smdata->tasklist, MUIM_List_Clear);

    if (FillTaskInfoBuffer(taskinfobuffer))
    {
        struct TaskInfo * ti = (struct TaskInfo *)taskinfobuffer;
        
        for (; ti->Next != NULL; ti = ti->Next)
        {
            DoMethod(smdata->tasklist, MUIM_List_InsertSingle, ti, MUIV_List_Insert_Bottom);
        }
    }

    if (firstvis < XGET(smdata->tasklist, MUIA_List_Entries))
    {
        if ((XGET(smdata->tasklist, MUIA_List_Entries) - firstvis) > XGET(smdata->tasklist, MUIA_List_Visible))
            set(smdata->tasklist, MUIA_List_First, firstvis);
        else
            set(smdata->tasklist, MUIA_List_First, (XGET(smdata->tasklist, MUIA_List_Entries) - XGET(smdata->tasklist, MUIA_List_Visible)));
    }
    set(smdata->tasklist, MUIA_List_Quiet, FALSE);
}

AROS_UFH3(VOID, TasksListDisplayFunction,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(STRPTR *, strings, A2),
    AROS_UFHA(struct TaskInfo *, obj, A1))
{
    AROS_USERFUNC_INIT

    static TEXT bufprio[8];

    if (obj)
    {
        __sprintf(bufprio, "%ld", (LONG)obj->Priority);
        strings[0] = obj->Name;
        strings[1] = bufprio;
        strings[2] = obj->Type == NT_TASK ? (STRPTR)_(MSG_TASK) : (STRPTR)_(MSG_PROCESS);
    }
    else
    {
        strings[0] = (STRPTR)_(MSG_TASK_NAME);
        strings[1] = (STRPTR)_(MSG_TASK_PRIORITY);
        strings[2] = (STRPTR)_(MSG_TASK_TYPE);
    }

    AROS_USERFUNC_EXIT
}

static BOOL InitTasks()
{
    return TRUE;
}

static VOID DeInitTasks()
{
}

struct SysMonModule tasksmodule =
{
    .Init = InitTasks,
    .DeInit = DeInitTasks,
};

