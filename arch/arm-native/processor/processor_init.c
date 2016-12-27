/*
    Copyright © 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
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
    if ((ProcessorBase->Private1 = sysprocs) == NULL)
        return FALSE;

    for (i = 0; i < ProcessorBase->cpucount; i++)
    {
    	sysprocs[i] = AllocMem(sizeof(struct ARMProcessorInformation), MEMF_CLEAR);
    	if (!sysprocs[i])
    	    return FALSE;
#if defined(__AROSEXEC_SMP__)
    	if (i > 0)
	{
            NewCreateTask(TASKTAG_AFFINITY      , KrnGetCPUMask(i)              },
                          TASKTAG_PRI           , -127,
                          TASKTAG_PC            , ReadProcessorInformation,
                          TASKTAG_ARG1          , sysprocs[i],
                          TAG_DONE);
	}
        else
#else
        if (i == 0)
#endif
            ReadProcessorInformation(sysprocs[i]);
    }

    DUMPINFO(
    bug("[processor.ARM] Processor Details -:\n");
        for (i = 0; i < ProcessorBase->cpucount; i++)
        {
            bug("[processor.ARM] ");
            if (ProcessorBase->cpucount > 1)
                bug("#%d ", i);
            if (sysprocs[i]->FamilyString)
            {
                bug("%s ARM%s Processor Core\n", sysprocs[i]->Vendor, sysprocs[i]->FamilyString);
            }
            else
            {
                bug("%s ARM Processor Core (unknown family)\n", sysprocs[i]->Vendor);
            }

            if (sysprocs[i]->Features1 & FEATF_FPU_VFP4)
            {
                bug("[processor.ARM]   VFPv4 Co-Processor\n");
            }
            else if (sysprocs[i]->Features1 & FEATF_FPU_VFP3_16)
            {
                bug("[processor.ARM]   VFPv3 [16Double] Co-Processor\n");
            }
            else if (sysprocs[i]->Features1 & FEATF_FPU_VFP3)
            {
                bug("[processor.ARM]   VFPv3 Co-Processor\n");
            }
            else if (sysprocs[i]->Features1 & FEATF_FPU_VFP2)
            {
                bug("[processor.ARM]   VFPv2 Co-Processor\n");
            }
            else
            {
                bug("[processor.ARM]   VFPv1 Co-Processor\n");            
            }

            if (sysprocs[i]->Features1 & FEATF_NEON)
            {
                bug("[processor.ARM]    NEON SIMD Extensions\n");
            }

            bug("[processor.ARM] Cache Info:\n");
            bug("[processor.ARM]   L1 Data   : %dKb\n", sysprocs[i]->L1DataCacheSize);
            bug("[processor.ARM]   L1 Instr. : %dKb\n", sysprocs[i]->L1InstructionCacheSize);
        }
    )

    return TRUE;
}

ADD2INITLIB(Processor_Init, 1);
