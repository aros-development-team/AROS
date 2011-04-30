/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <aros/symbolsets.h>

#include "processor_intern.h"
#include "processor_arch_intern.h"

LONG Processor_Init(struct ProcessorBase * ProcessorBase)
{
D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));

    struct SystemProcessors * sysprocs = 
        AllocVec(sizeof(struct SystemProcessors), MEMF_ANY | MEMF_CLEAR);

    if (sysprocs == NULL)
        return FALSE;

    ProcessorBase->Private1 = sysprocs;

    /* FIXME: Hardcodes for now */
    sysprocs->count = 1;
    ReadProcessorInformation(&sysprocs->processor);
    
    return TRUE;
}

ADD2INITLIB(Processor_Init, 1);
