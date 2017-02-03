/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <aros/symbolsets.h>

#include "processor_intern.h"
#include "processor_arch_intern.h"

LONG Processor_Init(struct ProcessorBase * ProcessorBase)
{
    struct X86ProcessorInformation **sysprocs;
    unsigned int i;

    D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));

    sysprocs = AllocVec(ProcessorBase->cpucount * sizeof(APTR), MEMF_ANY | MEMF_CLEAR);
    if (sysprocs == NULL)
        return FALSE;

    for (i = 0; i < ProcessorBase->cpucount; i++)
    {
    	sysprocs[i] = AllocMem(sizeof(struct X86ProcessorInformation), MEMF_CLEAR);
    	if (!sysprocs[i])
    	    return FALSE;
    }

    ProcessorBase->Private1 = sysprocs;

    /* Boot CPU is number 0. Fill in its data. */
    ReadProcessorInformation(sysprocs[0]);
    
    return TRUE;
}

ADD2INITLIB(Processor_Init, 1);
