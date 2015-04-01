/*
    Copyright © 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * RasPi timer driver.
 *
 * We use the GPU Timer #1 to handle EClock updates, since GPU Timer #3 is used for VBlank,
 * and Timers #0 and #2 are used by the GPU itself.
 *
 */

#define DEBUG 0

#include <aros/bootloader.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>
#include <proto/arossupport.h>
#include <proto/bootloader.h>
#include <proto/exec.h>
#include <proto/kernel.h>


#include "timer_intern.h"
#include "timer_macros.h"

#include <stdlib.h>
#include <string.h>

/* Timer 1 (EClock) interrupt handler */
static void Timer1Tick(struct TimerBase *TimerBase, struct ExecBase *SysBase)
{
    unsigned int last_CLO, last_CHI;

    D(bug("[Timer] Timer1Tick()\n"));

    if (!(TimerBase) || !(SysBase))
    {
        bug("[Timer] Timer1Tick: Bad Params!\n");
        return;
    }

    last_CHI = TimerBase->tb_Platform.tbp_CHI;
    last_CLO = TimerBase->tb_Platform.tbp_CLO;
    
    /* Aknowledge and update our timer interrupt */
    TimerBase->tb_Platform.tbp_cs = *((volatile unsigned int *)(SYSTIMER_CS));
    TimerBase->tb_Platform.tbp_cs &= ~(1 << TICK_TIMER);
    *((volatile unsigned int *)(SYSTIMER_CS)) = TimerBase->tb_Platform.tbp_cs;
    TimerBase->tb_Platform.tbp_CHI = *((volatile unsigned int *)(SYSTIMER_CHI));
    TimerBase->tb_Platform.tbp_CLO = *((volatile unsigned int *)(SYSTIMER_CLO));

    D(bug("[Timer] Timer1Tick: Updating EClock..\n"));
    D(bug("[Timer] Timer1Tick:   diff_CHI = %d\n", (TimerBase->tb_Platform.tbp_CHI - last_CHI)));
    D(bug("[Timer] Timer1Tick:   diff_CLO = %d\n", (TimerBase->tb_Platform.tbp_CLO - last_CLO)));

    TimerBase->tb_Platform.tbp_TickRate.tv_secs  = 0;
    if ((TimerBase->tb_Platform.tbp_CLO - last_CLO) > 0)
        TimerBase->tb_Platform.tbp_TickRate.tv_micro = TimerBase->tb_Platform.tbp_CLO - last_CLO;
    else
        TimerBase->tb_Platform.tbp_TickRate.tv_micro = ((1000000 - last_CLO) + TimerBase->tb_Platform.tbp_CLO);

    /* Increment EClock value and process microhz requests */
    ADDTIME(&TimerBase->tb_CurrentTime, &TimerBase->tb_Platform.tbp_TickRate);
    ADDTIME(&TimerBase->tb_Elapsed, &TimerBase->tb_Platform.tbp_TickRate);
    TimerBase->tb_ticks_total++;

    D(bug("[Timer] Timer1Tick: Processing events.. \n"));

    handleMicroHZ(TimerBase, SysBase);
//    handleEClock(TimerBase, SysBase);

    D(bug("[Timer] Timer1Tick: Reconfiguring interrupt..\n"));

    TimerBase->tb_Platform.tbp_CLO = *((volatile unsigned int *)(SYSTIMER_CLO));
    TimerBase->tb_Platform.tbp_cs |= (1 << TICK_TIMER);
    *((volatile unsigned int *)(SYSTIMER_CS)) = TimerBase->tb_Platform.tbp_cs;
    *((volatile unsigned int *)(SYSTIMER_C0 + (TICK_TIMER * 4))) = (TimerBase->tb_Platform.tbp_CLO + (1000000 / TimerBase->tb_eclock_rate));

    D(bug("[Timer] Timer1Tick: Done..\n"));
}

/****************************************************************************************/

static int Timer_Init(struct TimerBase *TimerBase)
{
    D(bug("[Timer] Timer_Init: kernel.resource @ 0x%p\n", KernelBase));

    TimerBase->tb_Platform.tbp_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase);

    /* Install timer IRQ handler */
    TimerBase->tb_TimerIRQHandle = KrnAddIRQHandler(IRQ_TIMER0 + TICK_TIMER, Timer1Tick, TimerBase, SysBase);
    if (!TimerBase->tb_TimerIRQHandle)
    	return FALSE;

    D(bug("[Timer] Timer_Init: TimerIRQHandle @ 0x%p\n", TimerBase->tb_TimerIRQHandle));

    /* By default we want 100 Hz EClock */
    TimerBase->tb_eclock_rate = 100;

    /*
     * Since we are software-driven, we can just ask the user which
     * frequencies he wishes to use.
     */
    BootLoaderBase = OpenResource("bootloader.resource");
    if (BootLoaderBase)
    {
	struct List *args = GetBootInfo(BL_Args);

	if (args)
        {
            struct Node *node;

            for (node = args->lh_Head; node->ln_Succ; node = node->ln_Succ)
            {
		if (strncasecmp(node->ln_Name, "eclock=", 7) == 0)
		{
		    TimerBase->tb_eclock_rate = atoi(&node->ln_Name[7]);
		    break;
		}
            }
        }
    }

    /* Set ExecBase public field. */
    SysBase->ex_EClockFrequency = TimerBase->tb_eclock_rate;
    D(bug("[Timer] Timer frequency is %d\n", TimerBase->tb_eclock_rate));

    /* Calculate timer period in us */
    TimerBase->tb_Platform.tbp_TickRate.tv_secs  = 0;
    TimerBase->tb_Platform.tbp_TickRate.tv_micro = 1000000 / TimerBase->tb_eclock_rate;

    /* Start up GPU timer #TICK_TIMER */
    Forbid();
    TimerBase->tb_Platform.tbp_cs = *((volatile unsigned int *)(SYSTIMER_CS));
    TimerBase->tb_Platform.tbp_cs &= ~(1 << TICK_TIMER);
    *((volatile unsigned int *)(SYSTIMER_CS)) = TimerBase->tb_Platform.tbp_cs;
    TimerBase->tb_Platform.tbp_CHI = *((volatile unsigned int *)(SYSTIMER_CHI));
    TimerBase->tb_Platform.tbp_CLO = *((volatile unsigned int *)(SYSTIMER_CLO));
    TimerBase->tb_Platform.tbp_cs |= (1 << TICK_TIMER);
    *((volatile unsigned int *)(SYSTIMER_CS)) = TimerBase->tb_Platform.tbp_cs;
    *((volatile unsigned int *)(SYSTIMER_C0 + (TICK_TIMER * 4))) = (TimerBase->tb_Platform.tbp_CLO + TimerBase->tb_Platform.tbp_TickRate.tv_micro);
    Permit();

    D(bug("[Timer] Timer_Init: configured GPU timer %d\n", TICK_TIMER));

    return TRUE;
}

static int Timer_Expunge(struct TimerBase *TimerBase)
{
    D(bug("[Timer] Timer_Expunge()\n"));

    Forbid();
    TimerBase->tb_Platform.tbp_cs = *((volatile unsigned int *)(SYSTIMER_CS));
    TimerBase->tb_Platform.tbp_cs &= ~ (1 << TICK_TIMER);
    *((volatile unsigned int *)(SYSTIMER_CS)) = TimerBase->tb_Platform.tbp_cs;
    Permit();

    if (TimerBase->tb_TimerIRQHandle)
    	KrnRemIRQHandler(TimerBase->tb_TimerIRQHandle);

    return TRUE;
}

ADD2INITLIB(Timer_Init, 0)
ADD2EXPUNGELIB(Timer_Expunge, 0)
