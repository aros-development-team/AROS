#include <aros/symbolsets.h>
#include <exec/types.h>

#include "kernel_base.h"

static int cpu_Init(struct KernelBase *KernelBase)
{
    /* All x86-64 processors have SSE. 15 bytes are added for alignment. */
    KernelBase->kb_ContextSize = sizeof(struct AROSCPUContext) + sizeof(struct FPXContext) + 15;
    return TRUE;
}

ADD2INITLIB(cpu_Init, 5);
