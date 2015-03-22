/*
    Copyright � 2013, The AROS Development Team. All rights reserved.
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

#undef ARM_PERIIOBASE
extern uint32_t __arm_periiobase;
#define ARM_PERIIOBASE (__arm_periiobase)

/* We use own implementation of bug(), so we don't need aros/debug.h */
#define DIRQ(x)
#define D(x)

void GPUSysTimerHandler(unsigned int timerno, void *unused1)
{
    unsigned int stc, cs;

    DIRQ(bug("[KRN] GPUSysTimerHandler(%d)\n", timerno));

    /* Aknowledge our timer interrupt */
    cs = *((volatile unsigned int *)(SYSTIMER_CS));
    cs &= ~ (1 << timerno);
    *((volatile unsigned int *)(SYSTIMER_CS)) = cs;

    /* Signal the Exec VBlankServer */
    if (SysBase && (SysBase->IDNestCnt < 0)) {
        core_Cause(INTB_VERTB, 1L << INTB_VERTB);
    }

    /* Refresh our timer interrupt */
    stc = *((volatile unsigned int *)(SYSTIMER_CLO));
    stc += VBLANK_INTERVAL;
    *((volatile unsigned int *)(SYSTIMER_CS)) = cs | (1 << timerno);
    *((volatile unsigned int *)(SYSTIMER_C0 + (timerno * 4))) = stc;

    DIRQ(bug("[KRN] GPUSysTimerHandler: Done..\n"));
}

void *KrnAddSysTimerHandler(struct KernelBase *KernelBase)
{
    struct IntrNode *GPUSysTimerHandle;
    unsigned int stc;

    D(bug("[KRN] KrnAddSysTimerHandler(%012p)\n", KernelBase));

    if ((GPUSysTimerHandle = AllocMem(sizeof(struct IntrNode), MEMF_PUBLIC|MEMF_CLEAR)) != NULL)
    {
        D(bug("[KRN] KrnAddSysTimerHandler: IntrNode @ 0x%p:\n", GPUSysTimerHandle));
        D(bug("[KRN] KrnAddSysTimerHandler: Using GPUTimer %d for VBlank\n", VBLANK_TIMER));

        GPUSysTimerHandle->in_Handler = GPUSysTimerHandler;
        GPUSysTimerHandle->in_HandlerData = VBLANK_TIMER;
        GPUSysTimerHandle->in_HandlerData2 = KernelBase;
        GPUSysTimerHandle->in_type = it_interrupt;
        GPUSysTimerHandle->in_nr = IRQ_TIMER0 + VBLANK_TIMER;

        ADDHEAD(&KernelBase->kb_Interrupts[IRQ_TIMER0 + VBLANK_TIMER], &GPUSysTimerHandle->in_Node);

        D(bug("[KRN] KrnAddSysTimerHandler: Enabling Hardware IRQ.. \n"));

        stc = *((volatile unsigned int *)(SYSTIMER_CLO));
        stc += VBLANK_INTERVAL;
        *((volatile unsigned int *)(SYSTIMER_CS)) = (1 << VBLANK_TIMER);
        *((volatile unsigned int *)(SYSTIMER_C0 + (VBLANK_TIMER * 4))) = stc;

        ictl_enable_irq(IRQ_TIMER0 + VBLANK_TIMER, KernelBase);
    }

    D(bug("[KRN] KrnAddSysTimerHandler: Done.. \n"));

    return GPUSysTimerHandle;
}
