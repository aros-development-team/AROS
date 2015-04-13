/*
    Copyright © 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_cpu.h>
#include <kernel_debug.h>
#include <kernel_interrupts.h>
#include <kernel_intr.h>
#include <kernel_objects.h>

#include "kernel_intern.h"

/* We use own implementation of bug(), so we don't need aros/debug.h */
#define DIRQ(x)
#define D(x)

void *KrnAddSysTimerHandler(struct KernelBase *KernelBase)
{
    struct IntrNode *SysTimerHandle = NULL;

    D(bug("[KRN] KrnAddSysTimerHandler(%012p)\n", KernelBase));

    if (__arm_arosintern.ARMI_InitTimer)
        SysTimerHandle = __arm_arosintern.ARMI_InitTimer(KernelBase);

    D(bug("[KRN] KrnAddSysTimerHandler: returning handle @ 0x%p \n", SysTimerHandle));

    return SysTimerHandle;
}
