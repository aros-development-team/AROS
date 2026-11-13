/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0

#include <aros/config.h>
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/mbox.h>
#include <resources/processor.h>

#include <hardware/bcm2708.h>
#include <hardware/videocore.h>

#include "processor_intern.h"
#include "processor_arch_intern.h"

#define MBOXMSG_WORDS   8
/* A whole cache line of its own; see MBOX_MSG_ALIGN in <proto/mbox.h>. */
#define MBOXMSG_SIZE    (MBOX_MSG_ALIGN + (MBOX_MSG_ALIGN - 1))

APTR MBoxBase = NULL;

static IPTR __arm_periiobase;
#define ARM_PERIIOBASE __arm_periiobase

/* The BCM2712 moved the mailbox within the peripheral window */
#define VCMB_OFFSET_BCM2712 0x013880
#define VCMB_ADDR           ((APTR)(__arm_periiobase + \
    ((__arm_periiobase == BCM2712_PERIIOBASE) ? VCMB_OFFSET_BCM2712 : VCMB_OFFSET)))

static UQUAD vcQueryClock(struct ProcessorBase *ProcessorBase, ULONG tag, ULONG clockid)
{
    unsigned int *msg_, *msg;
    UQUAD rate = 0;

    if (!MBoxBase)
    {
        /* On demand: mbox.resource (residentpri 88) inits after us (99). */
        if ((MBoxBase = OpenResource("mbox.resource")) == NULL)
            return 0;

        __arm_periiobase = (IPTR)KrnGetSystemAttr(KATTR_PeripheralBase);
    }

    if ((msg_ = AllocMem(MBOXMSG_SIZE, MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
        return 0;
    msg = (unsigned int *)((((IPTR)msg_) + (MBOX_MSG_ALIGN - 1)) & ~(IPTR)(MBOX_MSG_ALIGN - 1));

    msg[0] = AROS_LONG2LE(MBOXMSG_WORDS * 4);
    msg[1] = AROS_LONG2LE(VCTAG_REQ);
    msg[2] = AROS_LONG2LE(tag);
    msg[3] = AROS_LONG2LE(8);           /* value buffer size    */
    msg[4] = AROS_LONG2LE(4);           /* request length       */
    msg[5] = AROS_LONG2LE(clockid);
    msg[6] = 0;                         /* rate comes back here */
    msg[7] = 0;                         /* terminating tag      */

    /* MBoxCall, not Write+Read: it serialises against vc4gfx. */
    if (MBoxCall(VCMB_ADDR, VCMB_PROPCHAN, msg) == (volatile unsigned int *)msg)
        rate = (UQUAD)AROS_LE2LONG(msg[6]);

    FreeMem(msg_, MBOXMSG_SIZE);

    D(bug("[processor.AArch64] %s: tag %08x clock %u -> %u Hz\n", __func__, tag, clockid, (ULONG)rate));

    return rate;
}

VOID ReadMaxFrequencyInformation(struct ARMProcessorInformation * info)
{
    D(bug("[processor.AArch64] :%s()\n", __PRETTY_FUNCTION__));

    /* Left for the first query - the mailbox is not up yet here. */
    info->MaxCPUFrequency = 0;
}

UQUAD GetCurrentProcessorFrequency(struct ProcessorBase *ProcessorBase, struct ARMProcessorInformation * info)
{
    D(bug("[processor.AArch64] :%s()\n", __PRETTY_FUNCTION__));

    /* Cached; a failed query (zero) is retried. */
    if (info->CPUFrequency == 0)
    {
        if (info->MaxCPUFrequency == 0)
            info->MaxCPUFrequency = vcQueryClock(ProcessorBase, VCTAG_GETCLKMAX, VCCLOCK_ARM);

        /* Fall back to the setpoint on firmware without the measured tag. */
        if ((info->CPUFrequency = vcQueryClock(ProcessorBase, VCTAG_GETCLKMEASURED, VCCLOCK_ARM)) == 0)
            info->CPUFrequency = vcQueryClock(ProcessorBase, VCTAG_GETCLKRATE, VCCLOCK_ARM);

        if (info->CPUFrequency == 0)
            info->CPUFrequency = info->MaxCPUFrequency;
    }

    return info->CPUFrequency;
}
