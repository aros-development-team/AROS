/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Timer startup for the opensbi-riscv64 target.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/config.h>
#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <exec/devices.h>
#include <exec/interrupts.h>
#include <devices/timer.h>
#include <hardware/intbits.h>
#include <proto/exec.h>
#include <proto/execlock.h>
#include <proto/kernel.h>

#include "ticks.h"

#define KernelBase LIBBASE->tb_KernelBase

/* Runs when the deadline placed in the shared descriptor comes due */
static void TimerInt(struct TimerBase *LIBBASE, struct ExecBase *SysBase)
{
    EClockUpdate(LIBBASE);
    handleMicroHZ(LIBBASE, SysBase);
    Timer0Setup(LIBBASE);
}

static int hw_Init(struct TimerBase *LIBBASE)
{
    struct KrnPlatformTimer *kpt;

#if defined(__AROSEXEC_SMP__)
    struct ExecLockBase *ExecLockBase;
    if ((ExecLockBase = OpenResource("execlock.resource")) != NULL)
    {
        LIBBASE->tb_ExecLockBase = ExecLockBase;
        LIBBASE->tb_ListLock = AllocLock();
    }
#endif

    if (!KernelBase)
        return FALSE;

    kpt = (struct KrnPlatformTimer *)KrnGetSystemAttr(KATTR_PlatformTimer);
    if (!kpt || kpt == (APTR)-1 || !kpt->kpt_Frequency || kpt->kpt_IRQ == (ULONG)-1)
    {
        bug("[Timer] no usable platform timer\n");
        return FALSE;
    }

    LIBBASE->tb_Platform.tb_KPT = kpt;
    LIBBASE->tb_Platform.tb_TimerIRQNum = kpt->kpt_IRQ;
    LIBBASE->tb_Platform.tb_LastTime = KrnPlatformTimerRead();

    /* The time CSR is the EClock. VBlank is emulated at 50 Hz, changeable at runtime. */
    SysBase->VBlankFrequency    = 50;
    SysBase->ex_EClockFrequency = kpt->kpt_Frequency;
    LIBBASE->tb_eclock_rate     = kpt->kpt_Frequency;

    LIBBASE->tb_TimerIRQHandle = KrnAddIRQHandler(kpt->kpt_IRQ, TimerInt, LIBBASE, SysBase);
    if (!LIBBASE->tb_TimerIRQHandle)
        return FALSE;

    /* From here on the periodic interrupt is ours */
    Disable();
    kpt->kpt_Flags |= KPTF_CLAIMED;
    Enable();

    D(bug("[Timer] EClock %u Hz, IRQ %u\n", kpt->kpt_Frequency, kpt->kpt_IRQ));

    LIBBASE->tb_vblank_timerequest.tr_node.io_Command = TR_ADDREQUEST;
    LIBBASE->tb_vblank_timerequest.tr_node.io_Device = &LIBBASE->tb_Device;
    LIBBASE->tb_vblank_timerequest.tr_node.io_Unit   = (struct Unit *)UNIT_MICROHZ;
    LIBBASE->tb_vblank_timerequest.tr_time.tv_secs   = 0;
    LIBBASE->tb_vblank_timerequest.tr_time.tv_micro  = 1000000 / SysBase->VBlankFrequency;

    SendIO(&LIBBASE->tb_vblank_timerequest.tr_node);

    return TRUE;
}

static int hw_Expunge(struct TimerBase *LIBBASE)
{
    if (LIBBASE->tb_Platform.tb_KPT)
    {
        Disable();
        LIBBASE->tb_Platform.tb_KPT->kpt_Deadline = 0;
        LIBBASE->tb_Platform.tb_KPT->kpt_Flags &= ~KPTF_CLAIMED;
        Enable();
    }
    if (LIBBASE->tb_TimerIRQHandle)
        KrnRemIRQHandler(LIBBASE->tb_TimerIRQHandle);

    return TRUE;
}

ADD2INITLIB(hw_Init, 0)
ADD2EXPUNGELIB(hw_Expunge, 0)
