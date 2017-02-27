/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <exec/types.h>

#include "kernel_base.h"
#include <kernel_debug.h>

#define D(x)

static int cpu_Init(struct KernelBase *KernelBase)
{
    /* All x86-64 processors have SSE */
    D(bug("[Kernel] %s: KernelBase @ 0x%p\n", __func__, KernelBase);)
    D(bug("[Kernel] %s: context size = %u + %u\n", __func__, AROS_ROUNDUP2(sizeof(struct AROSCPUContext), 16), sizeof(struct FPXContext));)

    KernelBase->kb_ContextSize = AROS_ROUNDUP2(sizeof(struct AROSCPUContext), 16) + sizeof(struct FPXContext);

    D(bug("[Kernel] %s: CPU Context size = %u bytes\n", __func__, KernelBase->kb_ContextSize);)

    return TRUE;
}

ADD2INITLIB(cpu_Init, 5);
