/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include "sysmon_intern.h"

#include <proto/processor.h>
#include <resources/processor.h>
#include <proto/dos.h>

#include <clib/alib_protos.h>

#include <zune/graph.h>

#include "locale.h"

/* Processor information */
static IPTR processorcount;
APTR ProcessorBase;
#define SIMULATE_USAGE_FREQ 0

#if !defined(PROCDISPLAY_USEGAUGE)
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

   D(bug("[SysMon] %s(%d)\n", __func__, cpuNo);)

    *storage = 0;

    GetCPUInfo(tags);

    *storage = ((*storage >> 16) * 1000) >> 16;

    D(bug("[SysMon] %s: 0x%p = %d\n", __func__, storage, *storage);)

    return TRUE;

    AROS_USERFUNC_EXIT
}
#endif

/* Processor functions */
static BOOL InitProcessor(struct SysMonData *smdata)
{
#if SIMULATE_USAGE_FREQ
    processorcount = 4;
    return TRUE;
#else
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
#endif
}

static VOID DeInitProcessor(struct SysMonData *smdata)
{
}

ULONG GetProcessorCount()
{
    return processorcount;
}

VOID UpdateProcessorInformation(struct SysMonData * smdata)
{
    ULONG i;
    TEXT buffer[128];
#if defined(PROCDISPLAY_SINGLEGRAPH)
    ULONG totaluse = 0;
#endif

    for (i = 0; i < processorcount; i++)
    {
        ULONG usage = 0;
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
        usage = ((usage >> 16) * 1000) >> 16;
        
        frequency /= 1000000;
#endif
#if !defined(PROCDISPLAY_SINGLEGRAPH)
        __sprintf(buffer, "CPU %d\n%d.%d %% ", i, usage / 10, usage % 10);
#endif
#if (PROCDISPLAY_USEGAUGE)
        set(smdata->cpuusagegauges[i], MUIA_Gauge_Current, usage);
        set(smdata->cpuusagegauges[i], MUIA_Gauge_InfoText, (IPTR)buffer);
#else
#if !defined(PROCDISPLAY_SINGLEGRAPH)
        set(smdata->cpuusagegauges[i], MUIA_Graph_InfoText, (IPTR)buffer);
#else
        totaluse += usage;
#endif
#endif
        __sprintf(buffer, "%d MHz", (ULONG)frequency);
        set(smdata->cpufreqvalues[i], MUIA_Text_Contents, (IPTR)buffer);
    }
#if defined(PROCDISPLAY_SINGLEGRAPH)
    totaluse /= processorcount;
    __sprintf(buffer, "%d CPU's\n%d.%d %% ", processorcount, totaluse / 10, totaluse % 10);
    set(smdata->cpuusagegauge, MUIA_Graph_InfoText, (IPTR)buffer);
#endif
}

VOID UpdateProcessorStaticInformation(struct SysMonData * smdata)
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
        set(smdata->cpufreqlabels[i], MUIA_Text_Contents, buffer);
    }
}

struct SysMonModule processormodule =
{
    .Init = InitProcessor,
    .DeInit = DeInitProcessor,
};
