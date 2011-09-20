/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _SYSMON_INTERN_H
#define _SYSMON_INTERN_H

#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#define MEMORY_RAM  0
#define MEMORY_CHIP 1
#define MEMORY_FAST 2
#define MEMORY_VRAM 3
#define MEMORY_GART 4

struct SysMonData
{
    Object * application;
    Object * mainwindow;

    CONST_STRPTR tabs [4];

    struct Hook tasklistdisplayhook;
    struct Hook tasklistrefreshbuttonhook;

    Object * tasklist;

    Object ** cpuusagegauges;
    Object ** cpufreqlabels;
    Object ** cpufreqvalues;

    Object * memorysize[5];
    Object * memoryfree[5];
    
    IPTR tasklistautorefresh;
};

struct SysMonModule
{
    BOOL (*Init)();
    VOID (*DeInit)();
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
AROS_UFP3(VOID, TasksListDisplayFunction,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(STRPTR *, strings, A2),
    AROS_UFHA(struct TaskInfo *, obj, A1));

ULONG GetSIG_TIMER();
VOID SignalMeAfter(ULONG msecs);
#endif
