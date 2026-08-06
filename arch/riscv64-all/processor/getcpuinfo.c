/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: GetCPUInfo() - Provides information about installed CPUs
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/libcall.h>
#include <proto/utility.h>
#include <resources/processor.h>

#include "processor_intern.h"
#include "processor_arch_intern.h"

#include <proto/processor.h>

/* See rom/processor/getcpuinfo.c for documentation */

AROS_LH1(void, GetCPUInfo,
    AROS_LHA(struct TagItem *, tagList, A0),
    struct ProcessorBase *, ProcessorBase, 1, Processor)
{
    AROS_LIBFUNC_INIT

    struct TagItem *passedTag;
    const struct ProcessorTopology *topo = ProcessorBase->Topology;
    ULONG selected;

    selected = (ULONG)GetTagData(GCIT_SelectedProcessor, 0, tagList);
    if (selected >= ProcessorBase->cpucount)
        selected = 0;

    while ((passedTag = NextTagItem(&tagList)) != NULL)
    {
        switch (passedTag->ti_Tag)
        {
        case GCIT_NumberOfProcessors:
            *((ULONG *)passedTag->ti_Data) = ProcessorBase->cpucount;
            break;
        case GCIT_NumberOfPackages:
            *((ULONG *)passedTag->ti_Data) = topo ? topo->pt_Packages : 1;
            break;
        case GCIT_NumberOfClusters:
            *((ULONG *)passedTag->ti_Data) = topo ? topo->pt_Clusters : 1;
            break;
        case GCIT_NumberOfCores:
            *((ULONG *)passedTag->ti_Data) =
                topo ? topo->pt_Cores : ProcessorBase->cpucount;
            break;
        case GCIT_ThreadsPerCore:
            *((ULONG *)passedTag->ti_Data) = topo ? topo->pt_ThreadsPerCore : 1;
            break;
        case GCIT_SelectedProcessor:
            break;
        default:
            Processor_AnswerTag(ProcessorBase, selected, passedTag);
            break;
        }
    }

    AROS_LIBFUNC_EXIT
} /* GetCPUInfo() */
