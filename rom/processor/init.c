#include <aros/debug.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "processor_intern.h"

static LONG common_Init(struct ProcessorBase *ProcessorBase)
{
    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
    	return FALSE;

    ProcessorBase->cpucount = KrnGetCPUCount();
    D(bug("[processor] System has %u CPUs\n", ProcessorBase->cpucount));

    return TRUE;
}

ADD2INITLIB(common_Init, 0)
