/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * Windows-hosted timer driver.
 *
 * In Windows kernel we have two statically allocated timers.
 * Timer 0 is used by kernel for VBlank interrupts, timer 1 is our one.
 *
 * Our VBlank unit is driven by exec interrupt.
 *
 * TODO: Rewrite this implementation to use variable time intervals.
 */

#include <aros/bootloader.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>
#include <proto/arossupport.h>
#include <proto/bootloader.h>
#include <proto/exec.h>
#include <proto/hostlib.h>
#include <proto/kernel.h>

#include "timer_intern.h"
#include "timer_macros.h"
#include "timervblank.h"

#include <stdlib.h>
#include <string.h>

/* Timer 1 (EClock) interrupt handler */
static void TimerTick(struct TimerBase *TimerBase, struct ExecBase *SysBase)
{
    /* Increment EClock value and process microhz requests */
    ADDTIME(&TimerBase->tb_CurrentTime, &TimerBase->tb_Platform.tb_VBlankTime);
    ADDTIME(&TimerBase->tb_Elapsed, &TimerBase->tb_Platform.tb_VBlankTime);
    TimerBase->tb_ticks_total++;

    handleMicroHZ(TimerBase, SysBase);
}

/****************************************************************************************/

#define KernelBase TimerBase->tb_KernelBase

static int Timer_Init(struct TimerBase *TimerBase)
{
    APTR BootLoaderBase;
    int ret;

    HostLibBase = OpenResource("hostlib.resource");
    if (!HostLibBase)
    	return FALSE;

    TimerBase->tb_Platform.kernelHandle = HostLib_Open("Libs\\Host\\kernel.dll", NULL);
    if (!TimerBase->tb_Platform.kernelHandle)
    	return FALSE;

    TimerBase->tb_Platform.StartClock = HostLib_GetPointer(TimerBase->tb_Platform.kernelHandle, "StartClock", NULL);
    if (!TimerBase->tb_Platform.StartClock)
    	return FALSE;

    /* Install timer IRQ handler */
    TimerBase->tb_TimerIRQHandle = KrnAddIRQHandler(1, TimerTick, TimerBase, SysBase);
    if (!TimerBase->tb_TimerIRQHandle)
    	return FALSE;

    /* Set up VBlank handler */
    vblank_Init(TimerBase);

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
    D(bug("[Timer_Init] Timer frequency is %d\n", TimerBase->tb_eclock_rate));

    /* Calculate timer period in us */
    TimerBase->tb_Platform.tb_VBlankTime.tv_secs  = 0;
    TimerBase->tb_Platform.tb_VBlankTime.tv_micro = 1000000 / TimerBase->tb_eclock_rate;

    /* Start up timer 1 */
    Forbid();
    ret = TimerBase->tb_Platform.StartClock(1, TimerBase->tb_eclock_rate);
    Permit();

    D(bug("[Timer_Init] StartClock() returned %d\n", ret));
    return ret;
}

static int Timer_Expunge(struct TimerBase *TimerBase)
{
    if (!HostLibBase)
    	return TRUE;

    if (TimerBase->tb_TimerIRQHandle)
    	KrnRemIRQHandler(TimerBase->tb_TimerIRQHandle);

    if (TimerBase->tb_Platform.kernelHandle)
    	HostLib_Close(TimerBase->tb_Platform.kernelHandle, NULL);

    return TRUE;
}

ADD2INITLIB(Timer_Init, 0)
ADD2EXPUNGELIB(Timer_Expunge, 0)
