/*
    Copyright (C) 2017, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/processor.h>

#include <clib/alib_protos.h>

#include <resources/processor.h>
#include <zune/graph.h>

#include "sysmon_intern.h"

#include "processor.h"

struct ProcessorGraph_DATA
{
    IPTR        pg_Flags;
    IPTR        pg_CPUCount;
    Object      **pg_Graphs;
    struct Hook *pg_GraphReadHooks;
};

struct cpuPen {
    ULONG r;
    ULONG g;
    ULONG b;
};

#define PROCESSORPEN_COUNT 8
struct cpuPen processorPens[PROCESSORPEN_COUNT] =
{
  { 0x9B9B9B9B, 0xDEDEDEDE, 0xC0C0C0C0 },
  { 0x9E9E9E9E, 0x9B9B9B9B, 0xDEDEDEDE },
  { 0xDEDEDEDE, 0x9B9B9B9B, 0xBABABABA },
  { 0xDEDEDEDE, 0xB5B5B5B5, 0x9B9B9B9B },
  { 0xDEDEDEDE, 0x9B9B9B9B, 0x9E9E9E9E },
  { 0xA6A6A6A6, 0xDEDEDEDE, 0x9B9B9B9B },
  { 0x9B9B9B9B, 0xB2B2B2B2, 0xDEDEDEDE },
  { 0xE4E4E4E4, 0xEBEBEBEB, 0x2A2A2A2A }
};

#define PROCGF_SINGLE   (1 << 1)

#define SETUP_PROCGRAPH_INST_DATA       struct ProcessorGraph_DATA *data = INST_DATA(CLASS, self)

AROS_UFH3(IPTR, GraphReadProcessorValueFunc,
        AROS_UFHA(struct Hook *, procHook, A0),
        AROS_UFHA(IPTR *, storage, A2),
        AROS_UFHA(IPTR, cpuNo, A1))
{
    AROS_USERFUNC_INIT

    struct TagItem tags [] =
    {
        { GCIT_SelectedProcessor, cpuNo },
        { GCIT_ProcessorLoad, (IPTR)storage },
        { TAG_DONE, TAG_DONE }
    };

   D(bug("[SysMon:ProcGraph] %s(%d)\n", __func__, cpuNo);)

    *storage = 0;

    GetCPUInfo(tags);

    *storage = ((*storage >> 16) * 1000) >> 16;

    D(bug("[SysMon:ProcGraph] %s: 0x%p = %d\n", __func__, storage, *storage);)

    return TRUE;

    AROS_USERFUNC_EXIT
}

ULONG cpusperrow(ULONG x)
{
    register unsigned long op = x, res = 0, one;

    one = 1 << 30;
    while (one > op)
        one >>= 2;

    while (one != 0)
    {
        if (op >= res + one)
        {
            op -= res + one;
            res += one << 1;
        }
        res >>= 1;
        one >>= 2;
    }
    return res;
}

Object *ProcessorGraph__GraphObject()
{
    return GraphObject,
                    MUIA_Graph_InfoText, (IPTR) CPU_DEFSTR,
                    MUIA_Graph_ValueCeiling,    1000,
                    MUIA_Graph_ValueStep,       100,
                    MUIA_Graph_PeriodCeiling,   100000,
                    MUIA_Graph_PeriodStep,      10000,
                    MUIA_Graph_PeriodInterval,  1000,
                End;
}

Object *ProcessorGraph__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    BOOL singlemode = (BOOL)GetTagData(MUIA_ProcessorGrp_SingleMode, TRUE, message->ops_AttrList);
    ULONG processorcount = (ULONG)GetTagData(MUIA_ProcessorGrp_CPUCount, 1, message->ops_AttrList);
    Object *procGuage = NULL;
    ULONG cpupr = 1;

    D(bug("[SysMon:ProcGraph] %s()\n", __func__));

    if ((singlemode) || (processorcount == 1))
    {
        self = (Object *) DoSuperNewTags(CLASS, self, NULL,
                Child, (IPTR)(procGuage = ProcessorGraph__GraphObject()),
            TAG_MORE, (IPTR) message->ops_AttrList
        );
    }
    else
    {
        if (processorcount <= 4)
            cpupr = processorcount;
        else if ((cpupr = cpusperrow(processorcount)) < 4)
        {
            if (processorcount <= 4)
                cpupr = processorcount;
            else
                cpupr = 4;
        }

        self = (Object *) DoSuperNewTags(CLASS, self, NULL,
                MUIA_Group_Columns, cpupr,
            TAG_MORE, (IPTR) message->ops_AttrList
        );
    }
    
    if (self != NULL)
    {
        APTR procDataSource;
        IPTR i;

        SETUP_PROCGRAPH_INST_DATA;

        data->pg_Flags = 0;
        data->pg_CPUCount = processorcount;
        data->pg_GraphReadHooks = AllocVec(sizeof(struct Hook) * processorcount, MEMF_ANY | MEMF_CLEAR);

        if ((!singlemode) && (processorcount > 1))
            data->pg_Graphs = AllocVec(sizeof(Object *) * processorcount, MEMF_ANY | MEMF_CLEAR);
        else
        {
            data->pg_Graphs = (Object **)procGuage;
            data->pg_Flags |= PROCGF_SINGLE;
        }

        for (i = 0; i < processorcount; i++)
        {
            if (!(data->pg_Flags & PROCGF_SINGLE))
            {
                data->pg_Graphs[i] = GraphObject,
                                    MUIA_Graph_InfoText,        (IPTR) CPU_DEFSTR,
                                    MUIA_Graph_ValueCeiling,    1000,
                                    MUIA_Graph_ValueStep,       100,
                                    MUIA_Graph_PeriodCeiling,   100000,
                                    MUIA_Graph_PeriodStep,      10000,
                                    MUIA_Graph_PeriodInterval,  1000,
                                    MUIA_UserData,              i,
                                End;

                procGuage = data->pg_Graphs[i];
                procDataSource = (APTR)DoMethod(procGuage, MUIM_Graph_GetSourceHandle, 0);

                DoMethod(self, OM_ADDMEMBER, procGuage);
            }
            else
            {
                procDataSource = (APTR)DoMethod(procGuage, MUIM_Graph_GetSourceHandle, i);
                DoMethod(procGuage, MUIM_Graph_SetSourceAttrib, procDataSource, MUIV_Graph_Source_PenSrc, &processorPens[i % PROCESSORPEN_COUNT]);
            }

            data->pg_GraphReadHooks[i].h_Entry = (APTR)GraphReadProcessorValueFunc;
            data->pg_GraphReadHooks[i].h_Data = (APTR)i;

            DoMethod(procGuage, MUIM_Graph_SetSourceAttrib, procDataSource, MUIV_Graph_Source_ReadHook, &data->pg_GraphReadHooks[i]);
        }
        for (i %= cpupr; (i > 0) && (i < cpupr); i ++)
        {
            DoMethod(self, OM_ADDMEMBER, (IPTR)HVSpace);
        }
    }

    return self;
}

IPTR ProcessorGraph__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    SETUP_PROCGRAPH_INST_DATA;
    BOOL singlemode = (BOOL)(data->pg_Flags & PROCGF_SINGLE);
    IPTR retVal;

    D(bug("[SysMon:ProcGraph] %s()\n", __func__));

    retVal = DoSuperMethodA(CLASS, self, message);

    FreeVec(data->pg_GraphReadHooks);
    if (!singlemode)
        FreeVec(data->pg_Graphs);

    return retVal;
}

IPTR ProcessorGraph__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
//    SETUP_PROCGRAPH_INST_DATA;
    struct TagItem  *tstate = message->ops_AttrList, *tag;

    D(bug("[SysMon:ProcGraph] %s()\n", __func__));

    while ((tag = NextTagItem((struct TagItem **)&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        }
    }
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

IPTR ProcessorGraph__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    SETUP_PROCGRAPH_INST_DATA;
    IPTR *store = message->opg_Storage;
    IPTR retval = 0;

    D(bug("[SysMon:ProcGraph] %s()\n", __func__));

    switch (message->opg_AttrID)
    {
        case MUIA_ProcessorGrp_CPUCount:
            *store = (IPTR)data->pg_CPUCount;
            retval = TRUE;
            break;

        case MUIA_ProcessorGrp_SingleMode:
            *store = (IPTR)((BOOL)(data->pg_Flags & PROCGF_SINGLE));
            retval = TRUE;
            break;
    }

    if (!retval)
        retval = DoSuperMethodA(CLASS, self, (Msg) message);

    return retval;
}

IPTR ProcessorGraph__MUIM_ProcessorGrp_Update(Class *CLASS, Object *self, Msg message)
{
    SETUP_PROCGRAPH_INST_DATA;
    TEXT buffer[128];
    ULONG totaluse = 0;
    ULONG i;

    D(bug("[SysMon:ProcGraph] %s()\n", __func__));

    for (i = 0; i < data->pg_CPUCount; i++)
    {
        ULONG usage = 0;
        struct TagItem tags [] =
        {
            { GCIT_SelectedProcessor, (IPTR)i },
            { GCIT_ProcessorLoad, (IPTR)&usage },
            { TAG_DONE, TAG_DONE }
        };
        
        GetCPUInfo(tags);
        usage = ((usage >> 16) * 1000) >> 16;
        
        if (!(data->pg_Flags & PROCGF_SINGLE))
        {
            __sprintf(buffer, "CPU %d\n%d.%d %%", i, usage / 10, usage % 10);
            SET(data->pg_Graphs[i], MUIA_Graph_InfoText, (IPTR)buffer);
        }
        else
            totaluse += usage;
    }

    if (data->pg_Flags & PROCGF_SINGLE)
    {
        totaluse /= data->pg_CPUCount;
        __sprintf(buffer, "%d CPU's\n%d.%d %%", data->pg_CPUCount, totaluse / 10, totaluse % 10);
        SET((Object *)data->pg_Graphs, MUIA_Graph_InfoText, (IPTR)buffer);
    }
    return 0;
}

/*** Setup ******************************************************************/
PROCESSORGRP_CUSTOMCLASS
(
  ProcessorGraph, NULL, MUIC_Group, NULL,
  OM_NEW,                             struct opSet *,
  OM_DISPOSE,                         Msg,
  OM_SET,                             struct opSet *,
  OM_GET,                             struct opGet *,
  MUIM_ProcessorGrp_Update,           Msg
);
