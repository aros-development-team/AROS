/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
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

#define DINT(x)

/****************************************************************************************/

static void TimerInt(struct TimerBase *TimerBase, struct ExecBase *SysBase)
{
    DINT(bug("[Timer] %s(0x%p)\n", __func__, TimerBase));

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

    D(bug("[Timer] %s(0x%p)\n", __func__, LIBBASE));
    
    /* Set a mode that won't generate interrupts */
    outb(CH0|ACCESS_FULL|MODE_ONESHOT, PIT_CONTROL);

    return 0;

    AROS_INTFUNC_EXIT
}

/****************************************************************************************/

static int hw_Init(struct TimerBase *LIBBASE)
{
    D(UBYTE chstatus;)
    D(bug("[Timer] %s(0x%p)\n", __func__, LIBBASE));
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

    D(bug("[Timer] Initializing base values...\n"));

    /* We have fixed EClock rate. VBlank will be emulated at 50Hz, can be changed at runtime. */
    SysBase->VBlankFrequency    = 50;
    SysBase->ex_EClockFrequency = 1193180;
    LIBBASE->tb_eclock_rate     = 1193180;
    LIBBASE->tb_prev_tick	= 0xFFFF;

    D(bug("[Timer] Initializing hardware...\n"));

    /* Start up the timer. Count the whole range for now. */
    LIBBASE->tb_Platform.tb_ReloadMin = 2;
    LIBBASE->tb_Platform.tb_InitValue = 0xFFFF;
    LIBBASE->tb_Platform.tb_ReloadValue = 0x5D38; // 23864

    D(bug("[Timer]     gate start state = %x\n", (inb(0x61) & 0x3)));
    if ((inb(0x61) & 0x3))
    {
        /* Sanity: make sure the timer is disabled (set PC speaker PIT GATE's off)
         * before we start changing anything
         */
        D(bug("[Timer] disabling CH0/CH2 ...\n"));
        outb((inb(0x61) & ~0x3), 0x61);
    }
    D(
        bug("[Timer] CH0 setup ..\n", chstatus);
        outb(READBACK | LATCH_COUNT | RB_CH0, PIT_CONTROL);                          /* Readback channel 0 status            */
        chstatus = inb(PIT_CH0);
        bug("[Timer]     initial status = %02x\n", chstatus);
    )
    outb(CH0 | ACCESS_FULL | MODE_SW_STROBE, PIT_CONTROL);                      /* Software strobe mode, 16-bit access  */
    D(
        outb(READBACK | LATCH_COUNT | RB_CH0, PIT_CONTROL);                          /* Readback channel 0 status            */
        chstatus = inb(PIT_CH0);
        bug("[Timer]     configured status = %02x\n", chstatus);
        bug("[Timer] writing %04x to CH0 data port...\n", LIBBASE->tb_Platform.tb_InitValue);
    )
    ch_write(LIBBASE->tb_Platform.tb_InitValue, PIT_CH0);
    D(bug("[Timer]    started\n"));

    outb((inb(0x61) & 0xfd) | (1 << 0), 0x61);                                  /* Enable the timer (set PC speaker PIT GATE on) */
    D(bug("[Timer]    IRQ enabled\n"));

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
    D(bug("[Timer] %s(0x%p)\n", __func__, LIBBASE));

    KrnRemIRQHandler(LIBBASE->tb_TimerIRQHandle);
    RemResetCallback(&LIBBASE->tb_ResetHandler);

    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(hw_Init, 0)
ADD2EXPUNGELIB(hw_Expunge, 0)
