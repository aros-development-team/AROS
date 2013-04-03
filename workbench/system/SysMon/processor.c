#include "sysmon_intern.h"

#include <proto/processor.h>
#include <resources/processor.h>
#include <proto/dos.h>

#include <clib/alib_protos.h>

#include "locale.h"

/* Processor information */
static ULONG processorcount;
APTR ProcessorBase;
#define SIMULATE_USAGE_FREQ 0

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
        set(smdata->cpuusagegauges[i], MUIA_Gauge_Current, usage);
        set(smdata->cpuusagegauges[i], MUIA_Gauge_InfoText, (IPTR)buffer);
        __sprintf(buffer, "%d MHz", (ULONG)frequency);
        set(smdata->cpufreqvalues[i], MUIA_Text_Contents, (IPTR)buffer);
    }
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
