/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <exec/types.h>

#include "kernel_base.h"
#include <kernel_debug.h>

static int cpu_Init(struct KernelBase *KernelBase)
{
    /* All x86-64 processors have SSE */
    bug("[KRN] KernelBase @ 0x%p\n", KernelBase);
    bug("[KRN] context size = %u + %u\n", AROS_ROUNDUP2(sizeof(struct AROSCPUContext), 16), sizeof(struct FPXContext));
    KernelBase->kb_ContextSize = AROS_ROUNDUP2(sizeof(struct AROSCPUContext), 16) + sizeof(struct FPXContext);
    bug("[KRN] CPU Context size = %u bytes\n", KernelBase->kb_ContextSize);
    return TRUE;
}

ADD2INITLIB(cpu_Init, 5);
