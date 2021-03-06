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

#include "sysmon_intern.h"

#include "processor.h"

struct ProcessorGauge_DATA
{
    IPTR        pg_CPUCount;
    Object      **pg_Gauges;
};

#define SETUP_PROCGAUGE_INST_DATA       struct ProcessorGauge_DATA *data = INST_DATA(CLASS, self)

Object *ProcessorGauge__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    ULONG processorcount = (ULONG)GetTagData(MUIA_ProcessorGrp_CPUCount, 1, message->ops_AttrList);

    D(bug("[SysMon:ProcGauge] %s()\n", __func__));

    self = (Object *) DoSuperNewTags(CLASS, self, NULL,
            MUIA_Group_Columns, processorcount + 2,
        TAG_MORE, (IPTR) message->ops_AttrList
    );
    
    if (self != NULL)
    {
        int i;

        SETUP_PROCGAUGE_INST_DATA;

        data->pg_CPUCount = processorcount;
        data->pg_Gauges = AllocMem(sizeof(Object *) * processorcount, MEMF_ANY | MEMF_CLEAR);

        DoMethod(self, OM_ADDMEMBER, (IPTR)HVSpace);

        for (i = 0; i < processorcount; i++)
        {
            data->pg_Gauges[i] = GaugeObject,
                            GaugeFrame,
                            MUIA_Gauge_InfoText, (IPTR) CPU_DEFSTR,
                            MUIA_Gauge_Horiz, FALSE, MUIA_Gauge_Current, 0,
                            MUIA_Gauge_Max, 1000,
                            MUIA_UserData, i,
                        End;

            DoMethod(self, OM_ADDMEMBER, data->pg_Gauges[i]);
        }

        DoMethod(self, OM_ADDMEMBER, (IPTR)HVSpace);
    }

    return self;
}

IPTR ProcessorGauge__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    D(bug("[SysMon:ProcGauge] %s()\n", __func__));

    return DoSuperMethodA(CLASS, self, message);
}

IPTR ProcessorGauge__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
//    SETUP_PROCGAUGE_INST_DATA;
    struct TagItem  *tstate = message->ops_AttrList, *tag;

    D(bug("[SysMon:ProcGauge] %s()\n", __func__));

    while ((tag = NextTagItem((struct TagItem **)&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        }
    }
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

IPTR ProcessorGauge__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    SETUP_PROCGAUGE_INST_DATA;
    IPTR *store = message->opg_Storage;
    IPTR retval = 0;

    D(bug("[SysMon:ProcGauge] %s()\n", __func__));

    switch (message->opg_AttrID)
    {
        case MUIA_ProcessorGrp_CPUCount:
            *store = (IPTR)data->pg_CPUCount;
            retval = TRUE;
            break;

        case MUIA_ProcessorGrp_SingleMode:
            *store = (IPTR)FALSE;
            retval = TRUE;
            break;
    }

    if (!retval)
        retval = DoSuperMethodA(CLASS, self, (Msg) message);

    return retval;
}

IPTR ProcessorGauge__MUIM_ProcessorGrp_Update(Class *CLASS, Object *self, Msg message)
{
    SETUP_PROCGAUGE_INST_DATA;
    TEXT buffer[128];
    ULONG i, usage;

    D(bug("[SysMon:ProcGauge] %s()\n", __func__));

    for (i = 0; i < data->pg_CPUCount; i++)
    {
#if SIMULATE_USAGE_FREQ
        struct DateStamp ds;
        DateStamp(&ds);
        usage = (ds.ds_Tick * (i + 1)) % 100;
#else
        struct TagItem tags [] =
        {
            { GCIT_SelectedProcessor, (IPTR)i },
            { GCIT_ProcessorLoad, (IPTR)&usage },
            { TAG_DONE, TAG_DONE }
        };

        usage = 0;
        GetCPUInfo(tags);
        usage = ((usage >> 16) * 1000) >> 16;
#endif
        __sprintf(buffer, "CPU %d\n%d.%d %%", i, usage / 10, usage % 10);
        set(data->pg_Gauges[i], MUIA_Gauge_Current, usage);
        set(data->pg_Gauges[i], MUIA_Gauge_InfoText, (IPTR)buffer);
    }
    return 0;
}

/*** Setup ******************************************************************/
PROCESSORGRP_CUSTOMCLASS
(
  ProcessorGauge, NULL, MUIC_Group, NULL,
  OM_NEW,                             struct opSet *,
  OM_DISPOSE,                         Msg,
  OM_SET,                             struct opSet *,
  OM_GET,                             struct opGet *,
  MUIM_ProcessorGrp_Update,           Msg
);
