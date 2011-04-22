/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Timer startup and device commands, generic hardware-independent version
*/

/****************************************************************************************/

#include <aros/kernel.h>
#include <exec/types.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <exec/devices.h>
#include <exec/alerts.h>
#include <exec/initializers.h>
#include <devices/timer.h>
#include <hardware/intbits.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/timer.h>

#include <aros/symbolsets.h>
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "timer_macros.h"

#define KernelBase LIBBASE->tb_KernelBase

void TimerIRQ(struct TimerBase *TimerBase, struct ExecBase *SysBase);

/* exec.library VBlank interrupt handler  */
AROS_UFH4(static ULONG, VBlankInt,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(struct TimerBase *, TimerBase, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    /*
     * First increment the current time. No need to Disable() here as
     * there are no other interrupts that are allowed to interrupt us
     * that can do anything with this.
     */
    ADDTIME(&TimerBase->tb_CurrentTime, &TimerBase->tb_VBlankTime);
    ADDTIME(&TimerBase->tb_Elapsed, &TimerBase->tb_VBlankTime);
    TimerBase->tb_ticks_total++;

    /*
     * Now go to process requests.
     * We are called at rather low rate, so don't bother and check both queues.
     */
    checkUnit(TimerBase, &TimerBase->tb_Lists[UNIT_MICROHZ], SysBase);
    checkUnit(TimerBase, &TimerBase->tb_Lists[UNIT_VBLANK ], SysBase);

    return 0;

    AROS_USERFUNC_EXIT
}

/* Another entry point, for kernel.resource */
static void TimerTick(struct TimerBase *TimerBase, struct ExecBase *SysBase)
{
    /*
     * We duplicate code here in order to make things
     * a little bit faster.
     */
    ADDTIME(&TimerBase->tb_CurrentTime, &TimerBase->tb_VBlankTime);
    ADDTIME(&TimerBase->tb_Elapsed, &TimerBase->tb_VBlankTime);
    TimerBase->tb_ticks_total++;

    checkUnit(TimerBase, &TimerBase->tb_Lists[UNIT_MICROHZ], SysBase);
    checkUnit(TimerBase, &TimerBase->tb_Lists[UNIT_VBLANK ], SysBase);
}

/****************************************************************************************/

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
    LIBBASE->tb_eclock_rate = SysBase->ex_EClockFrequency;
    LIBBASE->tb_TimerIRQNum = -1;

    if (KernelBase && LIBBASE->tb_eclock_rate)
	LIBBASE->tb_TimerIRQNum = KrnGetSystemAttr(KATTR_TimerIRQ);

    if (LIBBASE->tb_TimerIRQNum == -1)
	LIBBASE->tb_eclock_rate = SysBase->VBlankFrequency;

    D(bug("[timer] Timer IRQ is %d, frequency is %u Hz\n", LIBBASE->tb_TimerIRQNum, LIBBASE->tb_eclock_rate));

    /* Calculate timer period in us */
    LIBBASE->tb_VBlankTime.tv_secs  = 0;
    LIBBASE->tb_VBlankTime.tv_micro = 1000000 / LIBBASE->tb_eclock_rate;

    D(kprintf("Timer period: %ld secs, %ld micros\n",
	LIBBASE->tb_VBlankTime.tv_secs, LIBBASE->tb_VBlankTime.tv_micro));

    /* Start up the interrupt server */
    if (LIBBASE->tb_TimerIRQNum == -1)
    {
	/*
	 * If we don't have periodic timer IRQ number from
	 * kernel.resource, we can possibly use exec VBlank
	 */
	struct Interrupt *is;

        /* Check if VBlank works */
	if (!KrnGetSystemAttr(KATTR_VBlankEnable))
    	    return FALSE;

	is = AllocMem(sizeof(struct Interrupt), MEMF_PUBLIC);
	
	if (is)
	{
	    is->is_Node.ln_Pri = 0;
	    is->is_Node.ln_Type = NT_INTERRUPT;
	    is->is_Node.ln_Name = (STRPTR)MOD_NAME_STRING;
	    is->is_Code = (void *)VBlankInt;
	    is->is_Data = LIBBASE;

	    AddIntServer(INTB_VERTB, is);
	}
	LIBBASE->tb_TimerIRQHandle = is;
    }
    else
	LIBBASE->tb_TimerIRQHandle = KrnAddIRQHandler(LIBBASE->tb_TimerIRQNum, TimerTick, LIBBASE, SysBase);

    return LIBBASE->tb_TimerIRQHandle ? TRUE : FALSE;
}

/****************************************************************************************/

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->tb_TimerIRQHandle)
    {
	if (LIBBASE->tb_TimerIRQNum == -1)
	{
	    RemIntServer(INTB_VERTB, LIBBASE->tb_TimerIRQHandle);
	    FreeMem(LIBBASE->tb_TimerIRQHandle, sizeof(struct Interrupt));
	}
	else
	    KrnRemIRQHandler(LIBBASE->tb_TimerIRQHandle);
    }
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0)
