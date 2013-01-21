/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_cpu.h>
#include <kernel_debug.h>
#include <kernel_interrupts.h>
#include <kernel_objects.h>

/* We use own implementation of bug(), so we don't need aros/debug.h */
#define D(x) x

void *KrnAddSysTimerHandler(uint8_t irq, irqhandler_t * handler, void * handlerData, void * handlerData2)
{
    struct IntrNode *GPUSysTimerHandle;
    struct KernelBase *KernelBase = (struct KernelBase *)handlerData2;

    D(bug("[KRN] KrnAddSysTimerHandler(%02x, %012p, %012p, %012p)\n", irq, handler, handlerData, handlerData2));

    if ((GPUSysTimerHandle = AllocMem(sizeof(struct IntrNode), MEMF_PUBLIC|MEMF_CLEAR)) != NULL)
    {
        D(bug("[KRN] KrnAddSysTimerHandler: IntrNode @ 0x%p:\n", GPUSysTimerHandle));

        GPUSysTimerHandle->in_Handler = handler;
        GPUSysTimerHandle->in_HandlerData = handlerData;
        GPUSysTimerHandle->in_HandlerData2 = handlerData2;
        GPUSysTimerHandle->in_type = it_interrupt;
        GPUSysTimerHandle->in_nr = irq;

        ADDHEAD(&KernelBase->kb_Interrupts[irq], &GPUSysTimerHandle->in_Node);

        ictl_enable_irq(irq, KernelBase);
    }
    return GPUSysTimerHandle;
}
