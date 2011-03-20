/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <aros/symbolsets.h>

#include "processor_intern.h"
#include "processor_arch_intern.h"

static VOID ReadProcessorInformation(struct M68KProcessorInformation * info)
{
    info->BrandStringBuffer[0] = '\0';
    info->BrandString = info->BrandStringBuffer;
}

LONG Processor_Init(struct ProcessorBase * ProcessorBase)
{
    struct SystemProcessors * sysprocs = 
        AllocVec(sizeof(struct SystemProcessors), MEMF_ANY | MEMF_CLEAR);

    if (sysprocs == NULL)
        return FALSE;

    ProcessorBase->Private1 = sysprocs;

    ReadProcessorInformation(&sysprocs->processor);
    
    return TRUE;
}

ADD2INITLIB(Processor_Init, 1);
