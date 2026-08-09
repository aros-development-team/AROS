/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

/*
 * RasPi timer driver.
 *
 * We use the GPU Timer #1 to handle EClock updates, since GPU Timer #3 is used for VBlank,
 * and Timers #0 and #2 are used by the GPU itself.
 *
 */

#define DEBUG 0

#include <aros/asmcall.h>
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

/*
 * Advance the clock by however much the free-running system timer counter has
 * moved since we last looked, then expire whatever that made due. Reading the
 * counter rather than assuming a fixed period means it does not matter which
 * interrupt brought us here, or how punctual it was.
 */
static void timer_ProcessTick(struct TimerBase *TimerBase, struct ExecBase *SysBase)
{
    unsigned int last_CLO;
    D(unsigned int last_CHI);

    D(last_CHI = TimerBase->tb_Platform.tbp_CHI);
    last_CLO = TimerBase->tb_Platform.tbp_CLO;

    TimerBase->tb_Platform.tbp_CHI = *((volatile unsigned int *)(SYSTIMER_CHI));
    TimerBase->tb_Platform.tbp_CLO = *((volatile unsigned int *)(SYSTIMER_CLO));

    D(bug("[Timer] %s: Updating EClock..\n", __func__));
    D(bug("[Timer] %s:   diff_CHI = %d\n", __func__, (TimerBase->tb_Platform.tbp_CHI - last_CHI)));
    D(bug("[Timer] %s:   diff_CLO = %d\n", __func__, (TimerBase->tb_Platform.tbp_CLO - last_CLO)));

    TimerBase->tb_Platform.tbp_TickRate.tv_secs  = 0;
    if ((TimerBase->tb_Platform.tbp_CLO - last_CLO) > 0)
        TimerBase->tb_Platform.tbp_TickRate.tv_micro = TimerBase->tb_Platform.tbp_CLO - last_CLO;
    else
        TimerBase->tb_Platform.tbp_TickRate.tv_micro = ((1000000 - last_CLO) + TimerBase->tb_Platform.tbp_CLO);

    /* Increment EClock value and process microhz requests */
    ADDTIME(&TimerBase->tb_CurrentTime, &TimerBase->tb_Platform.tbp_TickRate);
    ADDTIME(&TimerBase->tb_Elapsed, &TimerBase->tb_Platform.tbp_TickRate);
    TimerBase->tb_ticks_total++;

    D(bug("[Timer] %s: Processing events.. \n", __func__));

    handleMicroHZ(TimerBase, SysBase);
//    handleEClock(TimerBase, SysBase);
}

/* Timer 1 (EClock) interrupt handler */
static void Timer1Tick(struct TimerBase *TimerBase, struct ExecBase *SysBase)
{
    D(bug("[Timer] Timer1Tick()\n"));

    if (!(TimerBase) || !(SysBase))
    {
        bug("[Timer] Timer1Tick: Bad Params!\n");
        return;
    }

    /* Aknowledge our timer interrupt */
    *((volatile unsigned int *)(SYSTIMER_CS)) = (1 << TICK_TIMER); //TimerBase->tb_Platform.tbp_cs;

    timer_ProcessTick(TimerBase, SysBase);

    D(bug("[Timer] Timer1Tick: Reconfiguring interrupt..\n"));

    TimerBase->tb_Platform.tbp_CLO = *((volatile unsigned int *)(SYSTIMER_CLO));
    *((volatile unsigned int *)(SYSTIMER_C0 + (TICK_TIMER * 4))) = (TimerBase->tb_Platform.tbp_CLO + (1000000 / TimerBase->tb_eclock_rate));

    D(bug("[Timer] Timer1Tick: Done..\n"));
}

/*
 * BCM2711 only. The system timer's compare match is not delivered here - the
 * SoC routes GPU interrupts through the GIC, and the vendor device tree marks
 * the block disabled, so nothing arms that path. The kernel already runs the
 * ARM generic timer as its heartbeat and causes INTB_VERTB from it, so take
 * the tick from there instead. The counter itself is fine and keeps the clock
 * honest; only the interrupt is missing.
 */
AROS_INTH1(MicroHZVBlankInt, struct TimerBase *, TimerBase)
{
    AROS_INTFUNC_INIT

    timer_ProcessTick(TimerBase, SysBase);

    /* exec should continue with other servers */
    return 0;

    AROS_INTFUNC_EXIT
}

/****************************************************************************************/
int vblank_Init(struct TimerBase *LIBBASE);

static int Timer_Init(struct TimerBase *TimerBase)
{
    unsigned int timerIRQ;

    D(bug("[Timer] Timer_Init: kernel.resource @ 0x%p\n", KernelBase));

    TimerBase->tb_Platform.tbp_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase);

    /* Install timer IRQ handler */
    timerIRQ = IRQ_TIMER0 + TICK_TIMER;
    if (TimerBase->tb_Platform.tbp_periiobase == BCM2711_PERIIOBASE)
        timerIRQ += BCM2711_GPUIRQ_OFFSET;

    TimerBase->tb_TimerIRQHandle = KrnAddIRQHandler(timerIRQ, Timer1Tick, TimerBase, SysBase);
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
    TimerBase->tb_Platform.tbp_CHI = *((volatile unsigned int *)(SYSTIMER_CHI));
    TimerBase->tb_Platform.tbp_CLO = *((volatile unsigned int *)(SYSTIMER_CLO));
    *((volatile unsigned int *)(SYSTIMER_C0 + (TICK_TIMER * 4))) = (TimerBase->tb_Platform.tbp_CLO + TimerBase->tb_Platform.tbp_TickRate.tv_micro);
    Permit();

    vblank_Init(TimerBase);

    if (TimerBase->tb_Platform.tbp_periiobase == BCM2711_PERIIOBASE)
    {
        TimerBase->tb_Platform.tbp_MicroHZInt.is_Node.ln_Pri  = 0;
        TimerBase->tb_Platform.tbp_MicroHZInt.is_Node.ln_Type = NT_INTERRUPT;
        TimerBase->tb_Platform.tbp_MicroHZInt.is_Node.ln_Name = TimerBase->tb_Device.dd_Library.lib_Node.ln_Name;
        TimerBase->tb_Platform.tbp_MicroHZInt.is_Code         = (VOID_FUNC)MicroHZVBlankInt;
        TimerBase->tb_Platform.tbp_MicroHZInt.is_Data         = TimerBase;

        AddIntServer(INTB_VERTB, &TimerBase->tb_Platform.tbp_MicroHZInt);

        D(bug("[Timer] Timer_Init: microhz driven from VBlank (BCM2711)\n"));
    }

    D(bug("[Timer] Timer_Init: configured GPU timer %d\n", TICK_TIMER));

    return TRUE;
}

static int Timer_Expunge(struct TimerBase *TimerBase)
{
    D(bug("[Timer] Timer_Expunge()\n"));

    if (TimerBase->tb_TimerIRQHandle)
        KrnRemIRQHandler(TimerBase->tb_TimerIRQHandle);

    return TRUE;
}

ADD2INITLIB(Timer_Init, 0)
ADD2EXPUNGELIB(Timer_Expunge, 0)
