/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <aros/symbolsets.h>

#include "processor_intern.h"
#include "processor_arch_intern.h"

LONG Processor_Init(struct ProcessorBase * ProcessorBase)
{
    struct ARMProcessorInformation **sysprocs;
    unsigned int i;

    D(bug("[processor.ARM] :%s()\n", __PRETTY_FUNCTION__));

    sysprocs = AllocVec(ProcessorBase->cpucount * sizeof(APTR), MEMF_ANY | MEMF_CLEAR);
    if (sysprocs == NULL)
        return FALSE;

    for (i = 0; i < ProcessorBase->cpucount; i++)
    {
    	sysprocs[i] = AllocMem(sizeof(struct ARMProcessorInformation), MEMF_CLEAR);
    	if (!sysprocs[i])
    	    return FALSE;
    }

    ProcessorBase->Private1 = sysprocs;

    /* Boot CPU is number 0. Fill in its data. */
    ReadProcessorInformation(sysprocs[0]);

    D(bug("[processor.ARM] %s: Vendor %d '%s', Family %d\n", __PRETTY_FUNCTION__, sysprocs[0]->Vendor, sysprocs[0]->VendorID, sysprocs[0]->Family));
    D(bug("[processor.ARM] %s: L1DataC %d, L1InstrC %d\n", __PRETTY_FUNCTION__, sysprocs[0]->L1DataCacheSize, sysprocs[0]->L1InstructionCacheSize));

    return TRUE;
}

ADD2INITLIB(Processor_Init, 1);
