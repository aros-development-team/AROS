/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <exec/types.h>

#include "kernel_base.h"

/*
 * The job of this function is to probe the CPU and set up kb_ContextFlags
 * and kb_ContextSize.
 * kb_ContextFlags is whatever needs to be passed to KrnCreateContext() in
 * order to create a right thing. kb_ContextSize is total length of our
 * context area (including FPU data and private data). It is needed for
 * complete context save/restore during Exec exceptions processing
 */
static int cpu_Init(struct KernelBase *KernelBase)
{
    /*
     * On ARM the different thing is FPU type.
     * TODO: in future this can be extended to support more than
     * a single FPU type.
     */
    KernelBase->kb_ContextFlags = ARM_FPU_TYPE;
    KernelBase->kb_ContextSize  = sizeof(struct AROSCPUContext) + ARM_FPU_SIZE;

    return TRUE;
}
 
ADD2INITLIB(cpu_Init, 5);
