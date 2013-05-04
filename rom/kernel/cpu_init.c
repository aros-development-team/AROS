/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

/*
 * This is dummy routine, it needs to be here until transition to
 * unified CPUContext is complete. Without it exceptions on hosted
 * will not work
 */

#include <aros/symbolsets.h>
#include <exec/types.h>

#include "kernel_base.h"

static int cpu_Init(struct KernelBase *KernelBase)
{
    KernelBase->kb_ContextSize  = sizeof(struct AROSCPUContext);

    return TRUE;
}
 
ADD2INITLIB(cpu_Init, 5);
