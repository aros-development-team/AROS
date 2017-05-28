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

#include "processor.h"
#include "locale.h"

extern struct MUI_CustomClass                     *ProcessorGauge_CLASS;
extern struct MUI_CustomClass                     *ProcessorGraph_CLASS;

/* Processor information */
static IPTR processorcount;
APTR ProcessorBase;
#define SIMULATE_USAGE_FREQ 0

/* Processor functions */
static BOOL InitProcessor(struct SysMonData *smdata)
{
    if (!smdata->cpuusageclass)
    {
        smdata->cpuusageclass =
#if defined(PROCDISPLAY_USEGAUGE)
            ProcessorGauge_CLASS;
#else
            ProcessorGraph_CLASS;
#endif
        smdata->cpuusagesinglemode =
#if defined(PROCDISPLAY_SINGLEGRAPH)
                    TRUE;
#else
                    FALSE;
#endif
    }

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

Object *ProcessorGroupObject(struct SysMonData *smdata, IPTR count)
{
    Object *newGroupObj;

    newGroupObj = NewObject(smdata->cpuusageclass->mcc_Class, NULL,
                    MUIA_ProcessorGrp_CPUCount, count,
                    MUIA_ProcessorGrp_SingleMode, (IPTR)smdata->cpuusagesinglemode,
                TAG_DONE);

    return newGroupObj;
}

ULONG GetProcessorCount()
{
    return processorcount;
}

VOID UpdateProcessorInformation(struct SysMonData * smdata)
{
    ULONG i;
    TEXT buffer[128];

    DoMethod(smdata->cpuusageobj, MUIM_ProcessorGrp_Update);

    for (i = 0; i < processorcount; i++)
    {
        UQUAD frequency = 0;
#if SIMULATE_USAGE_FREQ
        struct DateStamp ds;
        DateStamp(&ds);
        frequency = ((ds.ds_Tick * (i + 1)) % 100) * 10;
#else
        struct TagItem tags [] = 
        {
            { GCIT_SelectedProcessor, (IPTR)i },
            { GCIT_ProcessorSpeed, (IPTR)&frequency },
            { TAG_DONE, TAG_DONE }
        };

        GetCPUInfo(tags);

        frequency /= 1000000;
#endif
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

AROS_UFH3(VOID, processorgaugehookfunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    struct SysMonData * smdata = h->h_Data;
    IPTR cpuCount = 0;
    Object *newGrp;

    D(bug("[SysMon:Processor] %s(0x%p)\n", __func__, smdata));

    if (smdata->cpuusageclass != ProcessorGauge_CLASS)
    {
        smdata->cpuusageclass = ProcessorGauge_CLASS;
        GET(smdata->cpuusageobj, MUIA_ProcessorGrp_CPUCount, &cpuCount);
        if ((newGrp = ProcessorGroupObject(smdata, cpuCount)) != NULL)
        {
            if (DoMethod(smdata->cpuusagegroup, MUIM_Group_InitChange))
            {
                DoMethod(smdata->cpuusagegroup, OM_ADDMEMBER, (IPTR)newGrp);
                DoMethod(smdata->cpuusagegroup, OM_REMMEMBER, (IPTR)smdata->cpuusageobj);
                DoMethod(smdata->cpuusagegroup, MUIM_Group_ExitChange);
                MUI_DisposeObject(smdata->cpuusageobj);
                smdata->cpuusageobj = newGrp;
            }
        }
    }

    AROS_USERFUNC_EXIT
}

AROS_UFH3(VOID, processorgraphhookfunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    struct SysMonData * smdata = h->h_Data;
    IPTR cpuCount = 0, graphMode = 0;
    Object *newGrp;

    D(bug("[SysMon:Processor] %s(0x%p)\n", __func__, smdata));

    GET(smdata->cpuusageobj, MUIA_ProcessorGrp_CPUCount, &cpuCount);
    GET(smdata->cpuusageobj, MUIA_ProcessorGrp_SingleMode, &graphMode);
    if ((smdata->cpuusageclass != ProcessorGraph_CLASS) || ((cpuCount > 1) && (!graphMode)))
    {
        smdata->cpuusageclass = ProcessorGraph_CLASS;
        smdata->cpuusagesinglemode = TRUE;
        if ((newGrp = ProcessorGroupObject(smdata, cpuCount)) != NULL)
        {
            if (DoMethod(smdata->cpuusagegroup, MUIM_Group_InitChange))
            {
                DoMethod(smdata->cpuusagegroup, OM_ADDMEMBER, (IPTR)newGrp);
                DoMethod(smdata->cpuusagegroup, OM_REMMEMBER, (IPTR)smdata->cpuusageobj);
                DoMethod(smdata->cpuusagegroup, MUIM_Group_ExitChange);
                MUI_DisposeObject(smdata->cpuusageobj);
                smdata->cpuusageobj = newGrp;
            }
        }
    }

    AROS_USERFUNC_EXIT
}

AROS_UFH3(VOID, processorgraphpercpuhookfunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT
    IPTR cpuCount = 0, graphMode = 0;
    Object *newGrp;

    struct SysMonData * smdata = h->h_Data;

    D(bug("[SysMon:Processor] %s(0x%p)\n", __func__, smdata));

    GET(smdata->cpuusageobj, MUIA_ProcessorGrp_CPUCount, &cpuCount);
    GET(smdata->cpuusageobj, MUIA_ProcessorGrp_SingleMode, &graphMode);
    if ((smdata->cpuusageclass != ProcessorGraph_CLASS) || (graphMode))
    {
        smdata->cpuusageclass = ProcessorGraph_CLASS;
        smdata->cpuusagesinglemode = FALSE;
        if ((newGrp = ProcessorGroupObject(smdata, cpuCount)) != NULL)
        {
            if (DoMethod(smdata->cpuusagegroup, MUIM_Group_InitChange))
            {
                DoMethod(smdata->cpuusagegroup, OM_ADDMEMBER, (IPTR)newGrp);
                DoMethod(smdata->cpuusagegroup, OM_REMMEMBER, (IPTR)smdata->cpuusageobj);
                DoMethod(smdata->cpuusagegroup, MUIM_Group_ExitChange);
                MUI_DisposeObject(smdata->cpuusageobj);
                smdata->cpuusageobj = newGrp;
            }
        }
    }

    AROS_USERFUNC_EXIT
}



struct SysMonModule processormodule =
{
    .Init = InitProcessor,
    .DeInit = DeInitProcessor,
};
