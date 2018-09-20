/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Timer startup and device commands
*/

#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

#include <aros/config.h>
#include <exec/devices.h>
#include <exec/interrupts.h>
#include <devices/timer.h>
#include <hardware/intbits.h>
#include <proto/exec.h>
#include <proto/execlock.h>
#include <proto/kernel.h>
#include <aros/symbolsets.h>
#include <asm/io.h>

#include "ticks.h"

#define KernelBase LIBBASE->tb_KernelBase

/****************************************************************************************/

static void TimerInt(struct TimerBase *TimerBase, struct ExecBase *SysBase)
{
    /*
     * Sync up with the hardware, we need the proper time value in order to
     * process our requests correctly.
     */
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

static AROS_INTH1(ResetHandler, struct TimerBase *, LIBBASE)
{
    AROS_INTFUNC_INIT

    /* Set a mode that won't generate interrupts */
    outb(CH0|ACCESS_FULL|MODE_ONESHOT, PIT_CONTROL);

    return 0;

    AROS_INTFUNC_EXIT
}

/****************************************************************************************/

static int hw_Init(struct TimerBase *LIBBASE)
{
#if defined(__AROSEXEC_SMP__)
    struct ExecLockBase *ExecLockBase;
    if ((ExecLockBase = OpenResource("execlock.resource")) != NULL)
    {
        LIBBASE->tb_ExecLockBase = ExecLockBase;
        LIBBASE->tb_ListLock = AllocLock();
    }
#endif

    /* We must have kernel.resource */
    D(bug("[Timer] KernelBase = 0x%p\n", KernelBase));
    if (!KernelBase)
    	return FALSE;

    /* Start up the interrupt server. We know that our HW timer is at IRQ 0 */
    LIBBASE->tb_TimerIRQHandle = KrnAddIRQHandler(0, TimerInt, LIBBASE, SysBase);
    D(bug("[Timer] IRQ handle = 0x%p\n", LIBBASE->tb_TimerIRQHandle));
    if (!LIBBASE->tb_TimerIRQHandle)
    	return FALSE;

    /* Install a reset handler */
    LIBBASE->tb_ResetHandler.is_Node.ln_Name =
        LIBBASE->tb_Device.dd_Library.lib_Node.ln_Name;
    LIBBASE->tb_ResetHandler.is_Code = (VOID_FUNC)ResetHandler;
    LIBBASE->tb_ResetHandler.is_Data = LIBBASE;
    AddResetCallback(&LIBBASE->tb_ResetHandler);

    D(bug("[Timer] Initializing hardware...\n"));

    /* We have fixed EClock rate. VBlank will be emulated at 50Hz, can be changed at runtime. */
    SysBase->VBlankFrequency    = 50;
    SysBase->ex_EClockFrequency = 1193180;
    LIBBASE->tb_eclock_rate     = 1193180;
    LIBBASE->tb_prev_tick	= 0xFFFF;

    /* Start up the timer. Count the whole range for now. */
    outb(CH0|ACCESS_FULL|MODE_SW_STROBE, PIT_CONTROL);  /* Software strobe mode, 16-bit access */
    ch_write(0xFFFF, PIT_CH0);

    /*
     * Start the timer2.
     * FIXME: This is not used by timer.device any more and must be removed.
     * However PS/2 port driver uses polled microsecond delays via channel 2,
     * and it relies on it being activated by us. This urgently needs to
     * be fixed!
     */
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

static int hw_Expunge(struct TimerBase *LIBBASE)
{
    KrnRemIRQHandler(LIBBASE->tb_TimerIRQHandle);
    RemResetCallback(&LIBBASE->tb_ResetHandler);

    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(hw_Init, 0)
ADD2EXPUNGELIB(hw_Expunge, 0)
