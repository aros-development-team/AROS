/*
    Copyright 2010-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _SYSMON_INTERN_H
#define _SYSMON_INTERN_H

#include <proto/exec.h>
#include <proto/intuition.h>

#include <libraries/mui.h>

#define MEMORY_RAM  0
#define MEMORY_CHIP 1
#define MEMORY_FAST 2
#define MEMORY_VRAM 3
#define MEMORY_GART 4

struct SysMonData
{
    struct Task *sm_Task;
    Object * application;
    Object * mainwindow;

    CONST_STRPTR tabs [4];

    struct Hook tasklistconstructhook;
    struct Hook tasklistdestructhook;
    struct Hook tasklistdisplayhook;
    struct Hook taskselectedhook;
    struct Hook tasklistrefreshbuttonhook;

    Object * tasklist;

    Object ** cpuusagegauges;
    Object ** cpufreqlabels;
    Object ** cpufreqvalues;

    Object * memorysize[5];
    Object * memoryfree[5];
    
    IPTR tasklistautorefresh;
    
    struct List sm_TaskList;
    struct Task *sm_TaskSelected;
    
    ULONG sm_TasksWaiting;
    ULONG sm_TasksReady;
    ULONG sm_TaskTotalRuntime;
};

struct SysMonModule
{
    BOOL (*Init)(struct SysMonData *);
    VOID (*DeInit)(struct SysMonData *);
};

extern struct SysMonModule memorymodule;
extern struct SysMonModule videomodule;
extern struct SysMonModule processormodule;
extern struct SysMonModule timermodule;
extern struct SysMonModule tasksmodule;

VOID UpdateMemoryInformation(struct SysMonData * smdata);
VOID UpdateMemoryStaticInformation(struct SysMonData * smdata);

VOID UpdateVideoInformation(struct SysMonData * smdata);
VOID UpdateVideoStaticInformation(struct SysMonData * smdata);

ULONG GetProcessorCount();
VOID UpdateProcessorInformation(struct SysMonData * smdata);
VOID UpdateProcessorStaticInformation(struct SysMonData * smdata);

VOID UpdateTasksInformation(struct SysMonData * smdata);
VOID UpdateTasksStaticInformation(struct SysMonData * smdata);

struct TaskInfo;

AROS_UFP3(struct TaskInfo *, TasksListConstructFunction,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(APTR, pool, A2),
    AROS_UFHA(struct Task *, curTask, A1));

AROS_UFP3(VOID, TasksListDestructFunction,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(APTR, pool, A2),
    AROS_UFHA(struct TaskInfo *, obj, A1));

AROS_UFP3(VOID, TasksListDisplayFunction,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(STRPTR *, strings, A2),
    AROS_UFHA(struct TaskInfo *, obj, A1));

AROS_UFP3(VOID, TaskSelectedFunction,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(APTR, msg, A1));

ULONG GetSIG_TIMER();
VOID SignalMeAfter(ULONG msecs);
#endif
