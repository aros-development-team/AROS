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

#define KernelBase LIBBASE->tb_KernelBase

AROS_UFP4(ULONG, VBlankInt,
    AROS_UFPA(ULONG, dummy, A0),
    AROS_UFPA(struct TimerBase *, TimerBase, A1),
    AROS_UFPA(ULONG, dummy2, A5),
    AROS_UFPA(struct ExecBase *, SysBase, A6)
);

void TimerIRQ(struct TimerBase *TimerBase, struct ExecBase *SysBase);

/****************************************************************************************/

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
    ULONG TimerPeriod = SysBase->ex_EClockFrequency;

    LIBBASE->tb_TimerIRQNum = -1;

    if (KernelBase && TimerPeriod)
	LIBBASE->tb_TimerIRQNum = KrnGetSystemAttr(KATTR_TimerIRQ);

    if (LIBBASE->tb_TimerIRQNum == -1)
	TimerPeriod = SysBase->VBlankFrequency;

    D(bug("[timer] Timer IRQ is %d, frequency is %u Hz\n", LIBBASE->tb_TimerIRQNum, TimerPeriod));

    /* Calculate timer period in us */
    LIBBASE->tb_VBlankTime.tv_secs  = 0;
    LIBBASE->tb_VBlankTime.tv_micro = 1000000 / TimerPeriod;

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
	LIBBASE->tb_TimerIRQHandle = KrnAddIRQHandler(LIBBASE->tb_TimerIRQNum, TimerIRQ, LIBBASE, SysBase);

    return LIBBASE->tb_TimerIRQHandle ? TRUE : FALSE;
}

/****************************************************************************************/

static int GM_UNIQUENAME(Open)
(
    LIBBASETYPEPTR LIBBASE,
    struct timerequest *tr,
    ULONG unitNum,
    ULONG flags
)
{
    /*
        Normally, we should check the length of the message and other
        such things, however the RKM documents an example where the
        length of the timerrequest isn't set, so we must not check
        this.

        This fixes bug SF# 741580
    */

    switch(unitNum)
    {
	case UNIT_VBLANK:
	case UNIT_MICROHZ:
	case UNIT_WAITUNTIL:
	    tr->tr_node.io_Error = 0;
	    tr->tr_node.io_Unit = (NULL + unitNum);
	    tr->tr_node.io_Device = (struct Device *)LIBBASE;
	    break;

	case UNIT_ECLOCK:
	case UNIT_WAITECLOCK:	
	default:
	    tr->tr_node.io_Error = IOERR_OPENFAIL;
    }

    return TRUE;
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
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0)
