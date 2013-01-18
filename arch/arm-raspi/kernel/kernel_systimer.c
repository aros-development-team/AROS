#include <aros/kernel.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_cpu.h>
#include <kernel_debug.h>
#include <kernel_interrupts.h>
#include <kernel_objects.h>

/* We use own implementation of bug(), so we don't need aros/debug.h */
#define D(x)

#include <proto/kernel.h>

struct IntrNode GPUSysTimerHandle;

void *KrnAddSysTimerHandler(uint8_t irq, irqhandler_t * handler, void * handlerData, void * handlerData2)
{
    D(bug("[KRN] KrnAddSysTimerHandler(%02x, %012p, %012p, %012p):\n", irq, handler, handlerData, handlerData2));

    GPUSysTimerHandle.in_Handler = handler;
    GPUSysTimerHandle.in_HandlerData = handlerData;
    GPUSysTimerHandle.in_HandlerData2 = handlerData2;
    GPUSysTimerHandle.in_type = it_interrupt;
    GPUSysTimerHandle.in_nr = irq;

    ADDHEAD(&KernelBase->kb_Interrupts[irq], &GPUSysTimerHandle.in_Node);

    ictl_enable_irq(irq, KernelBase);

    return &GPUSysTimerHandle;
}
