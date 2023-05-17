/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.
*/

#include <aros/symbolsets.h>
#include <exec/types.h>

#include "kernel_base.h"
#include "kernel_cpu.h"

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

    return TRUE;
}
 
ADD2INITLIB(cpu_Init, 5);
