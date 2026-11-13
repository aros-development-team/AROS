/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: CPU context sizing, AArch64 raspi version.
*/

#include <aros/symbolsets.h>
#include <exec/types.h>

#include "kernel_base.h"
#include "kernel_intern.h"

/*
 * Allocate the ExceptionContext and the FPU/NEON block as one chunk;
 * KrnCreateContext points fpuContext at the tail. The extra 8 bytes let
 * the FPU block be placed 16-byte aligned after the 280-byte context.
 */
static int cpu_InitContextSize(struct KernelBase *KernelBase)
{
    KernelBase->kb_ContextSize = sizeof(struct ExceptionContext) + 8
                               + sizeof(struct VFPContext);

    return TRUE;
}

ADD2INITLIB(cpu_InitContextSize, 5);
