/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <devices/timer.h>
#include <clib/alib_protos.h>
#include <proto/dos.h>
#include <proto/processor.h>
#include <resources/processor.h>
#include <exec/tasks.h>
#include <exec/execbase.h>

#include "locale.h"

#define VERSION "$VER: SysMon 1.0 (07.01.2011) ©2011 The AROS Development Team"

/* MUI information */
static Object * application;
static Object * mainwindow;

static Object * cpuusagegroup;
static Object ** cpuusagegauges;

static Object * cpufreqgroup;
static Object ** cpufreqlabels;
static Object ** cpufreqvalues;
static CONST_STRPTR tabs [] = {NULL, NULL, NULL};

static Object * tasklist;
static Object * tasklistrefreshbutton;
static Object * tasklistautorefreshcheckmark;
static struct Hook tasklistdisplayhook;
static struct Hook tasklistrefreshbuttonhook;
static IPTR tasklistautorefresh;

/* Processor information */
static ULONG processorcount;
APTR ProcessorBase;
#define SIMULATE_USAGE_FREQ 0

/* Timer information */
static struct MsgPort *     timerport = NULL;
static struct timerequest * timermsg = NULL;
static ULONG                SIG_TIMER = 0;

/* MUI Functions */
VOID UpdateCPUInformation()
{
    ULONG i;
    TEXT buffer[128];

    for (i = 0; i < processorcount; i++)
    {
        UBYTE usage = 0;
        UQUAD frequency = 0;
#if SIMULATE_USAGE_FREQ
        struct DateStamp ds;
        DateStamp(&ds);
        usage = (ds.ds_Tick * (i + 1)) % 100;
        frequency = usage * 10;
#else
        struct TagItem tags [] = 
        {
            { GCIT_SelectedProcessor, (IPTR)i },
            { GCIT_ProcessorSpeed, (IPTR)&frequency },
            { GCIT_ProcessorLoad, (IPTR)&usage },
            { TAG_DONE, TAG_DONE }
        };
        
        GetCPUInfo(tags);
        
        frequency /= 1000000;
#endif
        __sprintf(buffer, " CPU %d : %d %% ", i, usage);
        set(cpuusagegauges[i], MUIA_Gauge_Current, usage);
        set(cpuusagegauges[i], MUIA_Gauge_InfoText, (IPTR)buffer);
        __sprintf(buffer, "%d MHz", (ULONG)frequency);
        set(cpufreqvalues[i], MUIA_Text_Contents, (IPTR)buffer);
    }
}

/* Updated information which will not change through life of application */
VOID UpdateCPUStaticInformation()
{
    ULONG i;
    TEXT buffer[172];
    CONST_STRPTR modelstring;
    
    for (i = 0; i < processorcount; i++)
    {
#if SIMULATE_USAGE_FREQ
        modelstring = _(MSG_SIMULATED_CPU);
#else
        struct TagItem tags [] =
        {
            { GCIT_SelectedProcessor, (IPTR)i },
            { GCIT_ModelString, (IPTR)&modelstring },
            { TAG_DONE, TAG_DONE }
        };
        
        GetCPUInfo(tags);
#endif
        __sprintf(buffer, (STRPTR)_(MSG_PROCESSOR), i + 1, modelstring);
        set(cpufreqlabels[i], MUIA_Text_Contents, buffer);
    }
}

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
        while(*src != NULL)
        {
            ti->Name[namesize++] = *src;
            src++;
        }
    }
    
    *(ti->Name + namesize) = NULL; /* Terminate */

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

VOID UpdateTaskInformation()
{
    /* Clear prior to reading information, because list contains items
    from the taskinfobuffer. Once FillTaskInfoBuffer is executed old items
    are invalid and could crash list */
    set(tasklist, MUIA_List_Quiet, TRUE);
     
    DoMethod(tasklist, MUIM_List_Clear);

    if (FillTaskInfoBuffer(taskinfobuffer))
    {
        struct TaskInfo * ti = (struct TaskInfo *)taskinfobuffer;
        
        for (; ti->Next != NULL; ti = ti->Next)
        {
            DoMethod(tasklist, MUIM_List_InsertSingle, ti, MUIV_List_Insert_Bottom);
        }
    }

    set(tasklist, MUIA_List_Quiet, FALSE);
}

AROS_UFH3(VOID, tasklistdisplayfunction,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(STRPTR *, strings, A2),
    AROS_UFHA(struct TaskInfo *, obj, A1))
{
    AROS_USERFUNC_INIT

    static TEXT bufprio[8];

    if (obj)
    {
        __sprintf(bufprio, "%d", obj->Priority);
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

AROS_UFH3(VOID, tasklistrefreshbuttonfunction,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    UpdateTaskInformation();

    AROS_USERFUNC_EXIT
}

BOOL CreateApplication()
{
    Object * cpucolgroup;
    ULONG i;

    tabs[0] = _(MSG_TAB_TASKS);
    tabs[1] = _(MSG_TAB_CPU);

    tasklistdisplayhook.h_Entry = (APTR)tasklistdisplayfunction;
    tasklistrefreshbuttonhook.h_Entry = (APTR)tasklistrefreshbuttonfunction;

    application = ApplicationObject,
        MUIA_Application_Title, __(MSG_APP_NAME),
        MUIA_Application_Version, (IPTR) VERSION,
        MUIA_Application_Author, (IPTR) "Krzysztof Smiechowicz",
        MUIA_Application_Copyright, (IPTR)"©2011, The AROS Development Team",
        MUIA_Application_Base, (IPTR)"SYSMON",
        MUIA_Application_Description, __(MSG_APP_TITLE),
        SubWindow, 
            mainwindow = WindowObject,
                MUIA_Window_Title, __(MSG_WINDOW_TITLE),
                MUIA_Window_ID, MAKE_ID('S','Y','S','M'),
                MUIA_Window_Height, MUIV_Window_Height_Visible(45),
                MUIA_Window_Width, MUIV_Window_Width_Visible(35),
                WindowContents,
                    RegisterGroup(tabs),
                        Child, VGroup,
                            Child, ListviewObject, 
                                MUIA_Listview_List, tasklist = ListObject,
                                    ReadListFrame,
                                    MUIA_List_Format, "MIW=50 BAR,BAR,",
                                    MUIA_List_DisplayHook, &tasklistdisplayhook,
                                    MUIA_List_Title, (IPTR)TRUE,
                                End,
                            End,
                            Child, ColGroup(2),
                                Child, VGroup,
                                    Child, tasklistrefreshbutton = MUI_MakeObject(MUIO_Button, _(MSG_LIST_REFRESH)),
                                    Child, ColGroup(2),
                                        Child, Label(_(MSG_AUTO_REFRESH)),
                                        Child, tasklistautorefreshcheckmark = MUI_MakeObject(MUIO_Checkmark, NULL),
                                    End,
                                End,
                                Child, HVSpace,
                            End,
                        End,
                        Child, VGroup,
                            Child, cpuusagegroup = HGroup, GroupFrameT(_(MSG_USAGE)), 
                            End,
                            Child, VGroup, GroupFrameT(_(MSG_FREQUENCY)),
                                Child, cpufreqgroup = ColGroup(3), 
                                End,
                            End,
                        End,
                    End,
            End,
    End;
    
    if (!application)
        return FALSE;

    DoMethod(mainwindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        application, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(tasklistrefreshbutton, MUIM_Notify, MUIA_Pressed, FALSE,
        application, 2, MUIM_CallHook, (IPTR)&tasklistrefreshbuttonhook);

    DoMethod(tasklistautorefreshcheckmark, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        tasklistrefreshbutton, 3, MUIM_Set, MUIA_Disabled, MUIV_TriggerValue);

    DoMethod(tasklistautorefreshcheckmark, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        tasklistrefreshbutton, 3, MUIM_WriteLong, MUIV_TriggerValue, &tasklistautorefresh);

    /* Adding cpu usage gauges */
    cpucolgroup = ColGroup(processorcount + 1), End;
    
    cpuusagegauges = AllocVec(sizeof(Object *) * processorcount, MEMF_ANY | MEMF_CLEAR);
    
    for (i = 0; i < processorcount; i++)
    {
        cpuusagegauges[i] = GaugeObject, GaugeFrame, MUIA_Gauge_InfoText, (IPTR) " CPU XX : XXX% ",
                        MUIA_Gauge_Horiz, FALSE, MUIA_Gauge_Current, 0, 
                        MUIA_Gauge_Max, 100, End;
                        
        DoMethod(cpucolgroup, OM_ADDMEMBER, cpuusagegauges[i]);
    }
    
    DoMethod(cpucolgroup, OM_ADDMEMBER, (IPTR)HVSpace);
    
    DoMethod(cpuusagegroup, OM_ADDMEMBER, cpucolgroup);
    
    /* Adding cpu frequency labels */
    cpufreqlabels = AllocVec(sizeof(Object *) * processorcount, MEMF_ANY | MEMF_CLEAR);
    cpufreqvalues = AllocVec(sizeof(Object *) * processorcount, MEMF_ANY | MEMF_CLEAR);
    
    for (i = 0; i < processorcount; i++)
    {
        cpufreqlabels[i] = TextObject, MUIA_Text_PreParse, "\33l",
                        MUIA_Text_Contents, (IPTR)"", End;
        cpufreqvalues[i] = TextObject, TextFrame, MUIA_Background, MUII_TextBack,
                        MUIA_Text_PreParse, (IPTR)"\33l",
				        MUIA_Text_Contents, (IPTR)"", End;
        
        DoMethod(cpufreqgroup, OM_ADDMEMBER, cpufreqlabels[i]);
        DoMethod(cpufreqgroup, OM_ADDMEMBER, cpufreqvalues[i]);
        DoMethod(cpufreqgroup, OM_ADDMEMBER, (IPTR)HVSpace);
    }

    return TRUE;
}

VOID DisposeApplication()
{
    MUI_DisposeObject(application);
    
    FreeVec(cpuusagegauges);
    FreeVec(cpufreqlabels);
    FreeVec(cpufreqvalues);
}

/* Timer functions */
BOOL InitTimer()
{
    if((timerport = CreatePort(0,0)) == NULL)
        return FALSE;

    if((timermsg = (struct timerequest *) CreateExtIO(timerport, sizeof(struct timerequest))) == NULL)
    {
        DeletePort(timerport);
        timerport = NULL;
        return FALSE;
    }
        
    if(OpenDevice("timer.device", UNIT_VBLANK, ((struct IORequest *) timermsg), 0) != 0)
    {
        DeletePort(timerport);
        timerport = NULL;
        DeleteExtIO((struct IORequest *)timermsg);
        timermsg = NULL;
        return FALSE;
    }

    SIG_TIMER = 1 << timerport->mp_SigBit;

    return TRUE;
}

VOID SignalMeAfter(ULONG msecs)
{
    timermsg->tr_node.io_Command = TR_ADDREQUEST;
    timermsg->tr_time.tv_secs = msecs / 1000;
    timermsg->tr_time.tv_micro = (msecs % 1000) * 1000;
    SendIO((struct IORequest *)timermsg);
}

VOID DeInitTimer()
{
    if (timermsg != NULL)
    {
	    AbortIO((struct IORequest *)timermsg);
	    WaitIO((struct IORequest *)timermsg);
	    CloseDevice((struct IORequest *)timermsg);
	    DeleteExtIO((struct IORequest *)timermsg);
    }

	if(timerport != NULL) DeletePort(timerport);
}

/* Processor functions */
BOOL InitProcessor()
{
    ProcessorBase = OpenResource(PROCESSORNAME);
    
    if (ProcessorBase)
    {
        struct TagItem tags [] = 
        {
            { GCIT_NumberOfProcessors, (IPTR)&processorcount },
            { 0, (IPTR)NULL }
        };
        
        GetCPUInfo(tags);
        
        return TRUE;
    }


    return FALSE;
}

VOID DeInitProcessor()
{
}

int main()
{
    ULONG signals = 0;
    ULONG tasklistcounter = 0;

    Locale_Initialize();

#if SIMULATE_USAGE_FREQ
    processorcount = 4;
#else
    if(!InitProcessor())
        return 1;
#endif

    if (!InitTimer())
        return 1;

    if (!CreateApplication())
        return 1;

    UpdateCPUStaticInformation();
    UpdateCPUInformation();
    UpdateTaskInformation();
    SignalMeAfter(250);

    set(mainwindow, MUIA_Window_Open, TRUE);

    while (DoMethod(application, MUIM_Application_NewInput, &signals) != MUIV_Application_ReturnID_Quit)
    {
        if (signals)
        {
            signals = Wait(signals | SIGBREAKF_CTRL_C | SIG_TIMER);
            if (signals & SIGBREAKF_CTRL_C) break;
            if (signals & SIG_TIMER)
            {
                UpdateCPUInformation();
                if (tasklistautorefresh && ((tasklistcounter++ % 4) == 0)) 
                    UpdateTaskInformation();
                SignalMeAfter(250);
            }
        }
    }
    
    set(mainwindow, MUIA_Window_Open, FALSE);
    
    DisposeApplication();

    DeInitTimer();
    
    DeInitProcessor();

    Locale_Deinitialize();

    return 0;
}
