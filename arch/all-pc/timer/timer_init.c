/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Timer startup and device commands
*/

/****************************************************************************************/

#include <exec/devices.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <aros/symbolsets.h>
#include <asm/io.h>
#include <aros/debug.h>

#include "ticks.h"

#define KernelBase LIBBASE->tb_KernelBase

/****************************************************************************************/

static void TimerInt(struct TimerBase *TimerBase, struct ExecBase *SysBase)
{
    /* Sync up with the hardware */
    EClockUpdate(TimerBase);
    /*
     * Process MICROHZ requests.
     * VBLANK is emulated by us, so we don't check it here. We always
     * have one active request in this list, which will cause VBLANK checking.
     */
    handleMicroHZ(TimerBase, SysBase);
    /* Request next interrupt from the hardware */
    Timer0Setup(TimerBase);
}

/****************************************************************************************/

static int hw_Init(struct TimerBase *LIBBASE)
{
    /* We must have kernel.resource */
    D(bug("[Timer] KernelBase = 0x%p\n", KernelBase));
    if (!KernelBase)
    	return FALSE;

    /* Start up the interrupt server. We know that our HW timer is at IRQ 0 */
    LIBBASE->tb_TimerIRQHandle = KrnAddIRQHandler(0, TimerInt, LIBBASE, SysBase);
    D(bug("[Timer] IRQ handle = 0x%p\n", LIBBASE->tb_TimerIRQHandle));
    if (!LIBBASE->tb_TimerIRQHandle)
    	return FALSE;

    /* Shut off kernel's VBlank emulation */
    KrnSetSystemAttr(KATTR_VBlankEnable, FALSE);

    D(bug("[Timer] Initializing hardware...\n"));

    /* We have fixed EClock rate */
    SysBase->ex_EClockFrequency = 1193180;
    LIBBASE->tb_eclock_rate     = 1193180;
    LIBBASE->tb_prev_tick       = 0xffff;

    /* Start the timer2 */
    outb((inb(0x61) & 0xfd) | 1, 0x61); /* Enable the timer (set GATE on) */
    outb(0xb4, 0x43);			/* Binary mode on Timer2, count mode 2 */
    outb(0x00, 0x42);			/* We're counting whole range */
    outb(0x00, 0x42);

    /* Own VBlank EMU */
    D(bug("[Timer] Starting VBlank emulation (%u Hz)...\n", SysBase->VBlankFrequency));

    LIBBASE->tb_vblank_timerequest.tr_node.io_Command = TR_ADDREQUEST;
    LIBBASE->tb_vblank_timerequest.tr_node.io_Device = &LIBBASE->tb_Device;
    LIBBASE->tb_vblank_timerequest.tr_node.io_Unit   = (struct Unit *)UNIT_MICROHZ;    
    LIBBASE->tb_vblank_timerequest.tr_time.tv_secs   = 0;
    LIBBASE->tb_vblank_timerequest.tr_time.tv_micro  = 1000000 / SysBase->VBlankFrequency;

    SendIO(&LIBBASE->tb_vblank_timerequest.tr_node);

    D(bug("[Timer] Done\n"));

    return TRUE;
}

/****************************************************************************************/

/* This is executed after common code (we have priority = 10) */
static int hw_Open(struct TimerBase *LIBBASE, struct timerequest *tr, ULONG unitNum, ULONG flags)
{
    outb((inb(0x61) & 0xfd) | 1, 0x61); /* Enable the timer (set GATE on) */

    return TRUE;
}

/****************************************************************************************/

static int hw_Expunge(struct TimerBase *LIBBASE)
{
    outb((inb(0x61) & 0xfd) | 1, 0x61); /* Enable the timer (set GATE on) */
    KrnRemIRQHandler(LIBBASE->tb_TimerIRQHandle);
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(hw_Init, 0)
ADD2OPENDEV(hw_Open, 10)
ADD2EXPUNGELIB(hw_Expunge, 0)
