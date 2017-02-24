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

//#define PROCDISPLAY_USEGAUGE
//#define PROCDISPLAY_SINGLEGRAPH

#define MEMORY_RAM  0
#define MEMORY_CHIP 1
#define MEMORY_FAST 2
#define MEMORY_VRAM 3
#define MEMORY_GART 4

#define SYSMON_TABCOUNT         3

#if !defined(PROCDISPLAY_USEGAUGE)
AROS_UFP3(IPTR, GraphReadProcessorValueFunc,
        AROS_UFPA(struct Hook *, procHook, A0),
        AROS_UFPA(IPTR *, storage, A2),
        AROS_UFPA(IPTR, cpuNo, A1));
#endif

struct SysMonData
{
    struct Task *sm_Task;
    Object      *application;
    Object      *mainwindow;
    Object      *pages;

    CONST_STRPTR tabs[SYSMON_TABCOUNT + 1];

    struct Hook pageactivehook;
    struct Hook tasklistrefreshhook;
    STRPTR      tasklistinfobuf;

    Object      *tasklist;
    Object      *tasklistinfo;

#if defined (PROCDISPLAY_SINGLEGRAPH)
    Object      *cpuusagegauge;
#else
    Object      **cpuusagegauges;
#endif
#if !defined(PROCDISPLAY_USEGAUGE)
    struct Hook *cpureadhooks;
#endif
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

AROS_UFP3(APTR, TasksListDisplayFunction,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(STRPTR *, strings, A2),
    AROS_UFHA(struct TaskInfo *, obj, A1));

AROS_UFP3(VOID, TaskSelectedFunction,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(APTR, msg, A1));

AROS_UFP3(LONG, TaskCompareFunction,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(struct TaskInfo *, ti2, A2),
    AROS_UFHA(struct TaskInfo *, ti1, A1));

ULONG GetSIG_TIMER();
VOID SignalMeAfter(ULONG msecs);
#endif
