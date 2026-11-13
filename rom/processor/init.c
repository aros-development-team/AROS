/*
    Copyright (C) 1995-2013, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>

#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <resources/processor.h>

#include "processor_intern.h"

static LONG common_Init(struct ProcessorBase *ProcessorBase)
{
    struct ProcessorTopology *topo;
    struct ProcessorTopologyEntry *entry;
    unsigned int i;

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
        return FALSE;

    ProcessorBase->cpucount = KrnGetCPUCount();
    D(bug("[processor] System has %u CPUs\n", ProcessorBase->cpucount));

    /* Flat single-package default; arch init refines the entries */
    topo = AllocVec(sizeof(struct ProcessorTopology) +
                    ProcessorBase->cpucount * sizeof(struct ProcessorTopologyEntry),
                    MEMF_ANY | MEMF_CLEAR);
    if (topo)
    {
        entry = (struct ProcessorTopologyEntry *)(topo + 1);
        topo->pt_Count = ProcessorBase->cpucount;
        topo->pt_Packages = 1;
        topo->pt_Clusters = 1;
        topo->pt_Cores = ProcessorBase->cpucount;
        topo->pt_ThreadsPerCore = 1;
        topo->pt_Entries = entry;
        for (i = 0; i < ProcessorBase->cpucount; i++)
        {
            entry[i].pte_LogicalID = i;
            entry[i].pte_PhysicalID = i;
            entry[i].pte_CoreID = i;
        }
        ProcessorBase->Topology = topo;
    }

    return TRUE;
}

ADD2INITLIB(common_Init, 0)
