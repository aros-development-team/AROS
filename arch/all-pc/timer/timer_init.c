/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.

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
#include <resources/kernel.h>
#include <hardware/i8259a.h>

#include "ticks.h"

#define KernelBase LIBBASE->tb_KernelBase

#define DINT(x)
#define DTICK(x)

/****************************************************************************************/

static void TimerInt(struct TimerBase *LIBBASE, struct ExecBase *SysBase)
{
    UWORD prevtick = (UWORD)(LIBBASE->tb_prev_tick & 0xFFFF);
    DINT(
        bug("[Timer] %s(0x%p)\n", __func__, LIBBASE);
    )
    DTICK(
        bug("[Timer] %s: PrevTick = %p\n", __func__, prevtick);
    )

    /*
     * Sync up with the hardware, we need the proper time value in order to
     * process our requests correctly.
     */
    EClockUpdate(LIBBASE);
    DTICK(bug("[Timer] %s: Tick = %04x (diff == %04x)\n", __func__, prevtick, prevtick - LIBBASE->tb_prev_tick);)

    /*
     * Process MICROHZ requests.
     * VBLANK is emulated by us, so we don't check it here. We always
     * have one active request in this list, which will cause VBLANK checking.
     */
    handleMicroHZ(LIBBASE, SysBase);

    /* Request next interrupt from the hardware */
    Timer0Setup(LIBBASE);
}

static void TimerShutdownInt(struct TimerBase *LIBBASE, struct ExecBase *SysBase)
{
    DINT(
        bug("[Timer] %s(0x%p)\n", __func__, LIBBASE);
    )
    LIBBASE->tb_Platform.tb_Flags &= ~PCTIMER_FLAGF_ENABLED;
}

/****************************************************************************************/

static AROS_INTH1(ResetHandler, struct TimerBase *, LIBBASE)
{
    AROS_INTFUNC_INIT

    D(bug("[Timer] %s(0x%p)\n", __func__, LIBBASE));

    /*
     * End the timer interrupt(s).
     * First we make sure multitasking is disabled, and then swap out the
     * timers interrupt handler with the shutdown handler.
     * we then program the PIT to issue a terminal IRQ , but only set the first byte which stops the counter.
     * we then enable interrupts so that the final Interrupt is handled.
     * (all other interrupts should be disabled by this point)
     */
    Forbid();

    KrnRemIRQHandler(LIBBASE->tb_TimerIRQHandle);
    LIBBASE->tb_TimerIRQHandle = KrnAddIRQHandler(0, TimerShutdownInt, LIBBASE, SysBase);
    DINT(bug("[Timer] %s: Shutdown IRQ handle = 0x%p\n", __func__, LIBBASE->tb_TimerIRQHandle));

    outb(CH0 | ACCESS_FULL | MODE_TERMINAL, PIT_CONTROL);
    outb(CH0 | ACCESS_FULL | LATCH_COUNT, PIT_CONTROL);
    outb(0, PIT_CH0);

    KrnSti();
    KrnCli();
    Permit();

    DINT(bug("[Timer] %s: Timer shutdown complete\n", __func__);)
    
    return 0;

    AROS_INTFUNC_EXIT
}

/****************************************************************************************/

static int hw_Init(struct TimerBase *LIBBASE)
{
    D(
        UBYTE chstatus;
        bug("[Timer] %s(0x%p)\n", __func__, LIBBASE);
    )

#if defined(__AROSEXEC_SMP__)
    struct ExecLockBase *ExecLockBase;
    if ((ExecLockBase = OpenResource("execlock.resource")) != NULL)
    {
        LIBBASE->tb_ExecLockBase = ExecLockBase;
        LIBBASE->tb_ListLock = AllocLock();
    }
#endif

    /* We must have kernel.resource */
    D(bug("[Timer] %s: KernelBase = 0x%p\n", __func__, KernelBase));
    if (!KernelBase)
        return FALSE;

    D(bug("[Timer] %s: Initializing base values...\n", __func__));

    /* We have fixed EClock rate. VBlank will be emulated at 50Hz, can be changed at runtime. */
    SysBase->VBlankFrequency            = 50;
    SysBase->ex_EClockFrequency         = 1193180;
    LIBBASE->tb_eclock_rate             = 1193180;

    D(bug("[Timer] Master 8259 ELCREG contains %02x\n", inb(MASTER8259_ELCREG));)

    /* Start up the timer. Count the whole range for now. */
    LIBBASE->tb_Platform.tb_ReloadMin   = 2;
    LIBBASE->tb_Platform.tb_InitValue   = 0xFFFF;
    LIBBASE->tb_Platform.tb_ReloadValue = 0x5D38; // 23864
    LIBBASE->tb_prev_tick               = LIBBASE->tb_Platform.tb_InitValue;

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
        bug("[Timer] Initializing hardware...\n");
        /* Read channel 0 status            */
        outb(READBACK | LATCH_COUNT | RB_CH0, PIT_CONTROL);
        chstatus = inb(PIT_CH0);
        bug("[Timer] CH0 initial status = %02x\n", chstatus);
    )

    /* Start up the interrupt server. We know that our HW timer is at IRQ 0 */
    LIBBASE->tb_TimerIRQHandle = KrnAddIRQHandler(0, TimerInt, LIBBASE, SysBase);
    D(bug("[Timer] %s: IRQ handle = 0x%p\n", __func__, LIBBASE->tb_TimerIRQHandle));
    if (!LIBBASE->tb_TimerIRQHandle)
        return FALSE;
    LIBBASE->tb_Platform.tb_Flags |= PCTIMER_FLAGF_ENABLED;

    /* Install a reset handler */
    LIBBASE->tb_ResetHandler.is_Node.ln_Pri = -63;
    LIBBASE->tb_ResetHandler.is_Node.ln_Name =
        LIBBASE->tb_Device.dd_Library.lib_Node.ln_Name;
    LIBBASE->tb_ResetHandler.is_Code = (VOID_FUNC)ResetHandler;
    LIBBASE->tb_ResetHandler.is_Data = LIBBASE;
    AddResetCallback(&LIBBASE->tb_ResetHandler);

    D(bug("[Timer] CH0 setup ..\n", chstatus);)
    /* Set software strobe mode, 16-bit access  */
    outb(CH0 | ACCESS_FULL | MODE_SW_STROBE, PIT_CONTROL);
    D(
        /* Read channel 0 status            */
        outb(READBACK | LATCH_COUNT | RB_CH0, PIT_CONTROL);
        chstatus = inb(PIT_CH0);
        bug("[Timer]     configured status = %02x\n", chstatus);
        bug("[Timer] writing %04x to CH0 data port...\n", LIBBASE->tb_Platform.tb_InitValue);
    )
    ch_write(LIBBASE->tb_Platform.tb_InitValue, PIT_CH0);
    D(bug("[Timer]    started\n"));

    /* Enable the timer (set PC speaker PIT GATE on) */
    outb((inb(0x61) & 0xfd) | (1 << 0), 0x61);
    D(bug("[Timer]    IRQ enabled\n"));

    /* Timer VBlank EMU */
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
