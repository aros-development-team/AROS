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

    if (sysprocs[0]->Family >= CPUFAMILY_ARM_3)
    {
        int armvers = 3;

        if ((sysprocs[0]->Family & 0x8) != 0)
            armvers = 7;
        else if ((sysprocs[0]->Family & 0x7) == 0x7)
            armvers = 6;
        else if ((sysprocs[0]->Family & 0x7) > 2)
            armvers = 5;
        else if ((sysprocs[0]->Family & 0x7) > 0)
            armvers = 4;

        bug("[processor.ARM] %s ARMv%d Processor Core\n", sysprocs[0]->VendorID, armvers);
    }
    else
    {
        bug("[processor.ARM] %s ARM Processor core\n", sysprocs[0]->VendorID);
    }
    bug("[processor.ARM] Cache Info:\n");
    bug("[processor.ARM]   L1 Data   : %dKb\n", sysprocs[0]->L1DataCacheSize);
    bug("[processor.ARM]   L1 Instr. : %dKb\n", sysprocs[0]->L1InstructionCacheSize);

    return TRUE;
}

ADD2INITLIB(Processor_Init, 1);
