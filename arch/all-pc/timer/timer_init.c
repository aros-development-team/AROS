/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Timer startup and device commands
*/

/****************************************************************************************/

#include <exec/devices.h>
#include <exec/interrupts.h>
#include <devices/timer.h>
#include <hardware/intbits.h>
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

#ifdef __i386__
AROS_UFH4(static ULONG, VBlankInt,
	  AROS_UFHA(ULONG, dummy, A0),
	  AROS_UFHA(struct TimerBase *, TimerBase, A1),
	  AROS_UFHA(ULONG, dummy2, A5),
	  AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT
    
    TimerInt(TimerBase, SysBase);
    
    AROS_USERFUNC_EXIT
}
#endif

/****************************************************************************************/

static int hw_Init(struct TimerBase *LIBBASE)
{
#ifdef __i386__
    /*
     * i386-pc still has incomplete kernel.resource, and doesn't have
     * new IRQ API. So we still use INTB_TIMERTICK hack there.
     */
    LIBBASE->tb_VBlankInt.is_Node.ln_Pri = 0;
    LIBBASE->tb_VBlankInt.is_Node.ln_Type = NT_INTERRUPT;
    LIBBASE->tb_VBlankInt.is_Node.ln_Name = LIBBASE->tb_Device.dd_Library.lib_Node.ln_Name;
    LIBBASE->tb_VBlankInt.is_Code = (APTR)VBlankInt;
    LIBBASE->tb_VBlankInt.is_Data = LIBBASE;

    AddIntServer(INTB_TIMERTICK, &LIBBASE->tb_VBlankInt);
#else
    /* We must have kernel.resource */
    D(bug("[Timer] KernelBase = 0x%p\n", KernelBase));
    if (!KernelBase)
    	return FALSE;

    /* Start up the interrupt server. We know that our HW timer is at IRQ 0 */
    LIBBASE->tb_TimerIRQHandle = KrnAddIRQHandler(0, TimerInt, LIBBASE, SysBase);
    D(bug("[Timer] IRQ handle = 0x%p\n", LIBBASE->tb_TimerIRQHandle));
    if (!LIBBASE->tb_TimerIRQHandle)
    	return FALSE;

#endif
    D(bug("[Timer] Initializing hardware...\n"));

    /* We have fixed EClock rate. VBlank will be emulated at 50Hz, can be changed at runtime. */
    SysBase->VBlankFrequency    = 50;
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

#ifdef __i386__
    RemIntServer(INTB_TIMERTICK, &LIBBASE->tb_VBlankInt);
#else
    KrnRemIRQHandler(LIBBASE->tb_TimerIRQHandle);
#endif

    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(hw_Init, 0)
ADD2OPENDEV(hw_Open, 10)
ADD2EXPUNGELIB(hw_Expunge, 0)
