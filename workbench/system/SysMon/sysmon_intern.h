/*
    Copyright 2010-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _SYSMON_INTERN_H
#define _SYSMON_INTERN_H

#include <proto/exec.h>
#include <proto/intuition.h>
#include <libraries/mui.h>

#include "tasks.h"

/* uncomment to change the default processorgrp options */
//#define PROCDISPLAY_USEGAUGE
//#define PROCDISPLAY_SINGLEGRAPH

#define MEMORY_RAM  0
#define MEMORY_CHIP 1
#define MEMORY_FAST 2
#define MEMORY_VRAM 3
#define MEMORY_GART 4

#define SYSMON_TABCOUNT         3

struct SysMonData
{
    struct Task *sm_Task;
    Object      *application;
    Object      *mainwindow;
    Object      *pages;

    CONST_STRPTR tabs[SYSMON_TABCOUNT + 1];

    struct Hook pageactivehook;
    struct Hook                 tasklistrefreshhook;
    STRPTR                      tasklistinfobuf;

    Object                      *tasklist;
    Object                      *tasklistinfo;

    struct MUI_CustomClass      *cpuusageclass;
    Object                      *cpuusagegroup,
                                *cpuusageobj;
    BOOL                        cpuusagesinglemode;
    struct Hook                 cpuusagecmhooks[3];

    Object      **cpufreqlabels;
    Object      **cpufreqvalues;

    Object      *memorysize[5];
    Object      *memoryfree[5];

    STRPTR      msg_project;
    STRPTR      msg_refresh_speed;
    STRPTR      msg_fast;
    STRPTR      msg_normal;
    STRPTR      msg_slow;
    STRPTR      msg_taskreadywait;
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

VOID UpdateMemoryInformation(struct SysMonData *);
VOID UpdateMemoryStaticInformation(struct SysMonData *);

VOID UpdateVideoInformation(struct SysMonData *);
VOID UpdateVideoStaticInformation(struct SysMonData *);

ULONG GetProcessorCount();
VOID UpdateProcessorInformation(struct SysMonData *);
VOID UpdateProcessorStaticInformation(struct SysMonData *);
Object *ProcessorGroupObject(struct SysMonData *, IPTR);

VOID UpdateTasksInformation(struct SysMonData *);
VOID UpdateTasksStaticInformation(struct SysMonData *);

struct TaskInfo;

AROS_UFP3(struct TaskInfo *, TasksListConstructFunction,
    AROS_UFPA(struct Hook *, h,  A0),
    AROS_UFPA(APTR, pool, A2),
    AROS_UFPA(struct Task *, curTask, A1));

AROS_UFP3(VOID, TasksListDestructFunction,
    AROS_UFPA(struct Hook *, h,  A0),
    AROS_UFPA(APTR, pool, A2),
    AROS_UFPA(struct TaskInfo *, obj, A1));

AROS_UFP3(APTR, TasksListDisplayFunction,
    AROS_UFPA(struct Hook *, h,  A0),
    AROS_UFPA(STRPTR *, strings, A2),
    AROS_UFPA(struct TaskInfo *, obj, A1));

AROS_UFP3(VOID, TaskSelectedFunction,
    AROS_UFPA(struct Hook *, h, A0),
    AROS_UFPA(Object *, object, A2),
    AROS_UFPA(APTR, msg, A1));

AROS_UFP3(LONG, TaskCompareFunction,
    AROS_UFPA(struct Hook *, h, A0),
    AROS_UFPA(struct TaskInfo *, ti2, A2),
    AROS_UFPA(struct TaskInfo *, ti1, A1));

AROS_UFP3(VOID, processorgaugehookfunc,
    AROS_UFPA(struct Hook *, h, A0),
    AROS_UFPA(Object *, object, A2),
    AROS_UFPA(APTR, msg, A1));

AROS_UFP3(VOID, processorgraphhookfunc,
    AROS_UFPA(struct Hook *, h, A0),
    AROS_UFPA(Object *, object, A2),
    AROS_UFPA(APTR, msg, A1));

AROS_UFP3(VOID, processorgraphpercpuhookfunc,
    AROS_UFPA(struct Hook *, h, A0),
    AROS_UFPA(Object *, object, A2),
    AROS_UFPA(APTR, msg, A1));

ULONG GetSIG_TIMER();
VOID SignalMeAfter(ULONG msecs);
#endif
