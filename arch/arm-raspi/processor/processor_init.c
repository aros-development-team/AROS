/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <aros/symbolsets.h>

#include <resources/processor.h>

#include "processor_intern.h"
#include "processor_arch_intern.h"

#define DUMPINFO(a) a

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

    DUMPINFO(
        if (sysprocs[0]->FamilyString)
        {
            bug("[processor.ARM] %s ARM%s Processor Core\n", sysprocs[0]->VendorID, sysprocs[0]->FamilyString);
        }
        else
        {
            bug("[processor.ARM] %s ARM Processor Core (unknown family)\n", sysprocs[0]->VendorID);
        }

        if (sysprocs[0]->Features1 & FEATF_FPU_VFP4)
        {
            bug("[processor.ARM] VFPv4 Co-Processor\n");
        }
        else if (sysprocs[0]->Features1 & FEATF_FPU_VFP3_16)
        {
            bug("[processor.ARM] VFPv3 [16Double] Co-Processor\n");
        }
        else if (sysprocs[0]->Features1 & FEATF_FPU_VFP3)
        {
            bug("[processor.ARM] VFPv3 Co-Processor\n");
        }
        else if (sysprocs[0]->Features1 & FEATF_FPU_VFP2)
        {
            bug("[processor.ARM] VFPv2 Co-Processor\n");
        }
        else
        {
            bug("[processor.ARM] VFPv1 Co-Processor\n");            
        }

        if (sysprocs[0]->Features1 & FEATF_NEON)
        {
            bug("[processor.ARM]   NEON SIMD Extensions\n");
        }

        bug("[processor.ARM] Cache Info:\n");
        bug("[processor.ARM]   L1 Data   : %dKb\n", sysprocs[0]->L1DataCacheSize);
        bug("[processor.ARM]   L1 Instr. : %dKb\n", sysprocs[0]->L1InstructionCacheSize);
    )

    return TRUE;
}

ADD2INITLIB(Processor_Init, 1);
