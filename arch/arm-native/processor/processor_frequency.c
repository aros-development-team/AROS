/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0

#include <aros/config.h>
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/mbox.h>
#include <resources/processor.h>

#include <hardware/videocore.h>

#include "processor_intern.h"
#include "processor_arch_intern.h"

/* Measured rather than requested rate; not in videocore.h. The firmware
 * reports a setpoint the core is not actually running at. */
#define VCTAG_GETCLKRATE_MEASURED   0x00030047

#define MBOXMSG_WORDS   8
#define MBOXMSG_SIZE    (MBOXMSG_WORDS * 4 + 16)

APTR MBoxBase = NULL;

static IPTR __arm_periiobase;
#define ARM_PERIIOBASE __arm_periiobase

static UQUAD vcQueryClock(struct ProcessorBase *ProcessorBase, ULONG tag, ULONG clockid)
{
    unsigned int *msg_, *msg;
    UQUAD rate = 0;

    if (!MBoxBase)
    {
        /* On demand, not at init: processor.resource has residentpri 99
         * against mbox.resource's 88, so it does not exist yet. */
        if ((MBoxBase = OpenResource("mbox.resource")) == NULL)
            return 0;

        __arm_periiobase = (IPTR)KrnGetSystemAttr(KATTR_PeripheralBase);
    }

    /* The mailbox requires the message to be 16-byte aligned. */
    if ((msg_ = AllocMem(MBOXMSG_SIZE, MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
        return 0;
    msg = (unsigned int *)((((IPTR)msg_) + 15) & ~15);

    msg[0] = AROS_LONG2LE(MBOXMSG_WORDS * 4);
    msg[1] = AROS_LONG2LE(VCTAG_REQ);
    msg[2] = AROS_LONG2LE(tag);
    msg[3] = AROS_LONG2LE(8);           /* value buffer size    */
    msg[4] = AROS_LONG2LE(4);           /* request length       */
    msg[5] = AROS_LONG2LE(clockid);
    msg[6] = 0;                         /* rate comes back here */
    msg[7] = 0;                         /* terminating tag      */

    /* MBoxCall, not MBoxWrite+MBoxRead: it serialises the exchange under
     * the mailbox semaphore, which vc4gfx shares. */
    if (MBoxCall((APTR)VCMB_BASE, VCMB_PROPCHAN, msg) == (volatile unsigned int *)msg)
        rate = (UQUAD)AROS_LE2LONG(msg[6]);

    FreeMem(msg_, MBOXMSG_SIZE);

    D(bug("[processor.ARM] %s: tag %08x clock %u -> %u Hz\n", __func__, tag, clockid, (ULONG)rate));

    return rate;
}

VOID ReadMaxFrequencyInformation(struct ARMProcessorInformation * info)
{
    D(bug("[processor.ARM] :%s()\n", __PRETTY_FUNCTION__));

    /* Left for the first query - the mailbox is not up yet here. */
    info->MaxCPUFrequency = 0;
}

UQUAD GetCurrentProcessorFrequency(struct ProcessorBase *ProcessorBase, struct ARMProcessorInformation * info)
{
    D(bug("[processor.ARM] :%s()\n", __PRETTY_FUNCTION__));

    /* Sampled once, then cached - a mailbox round trip per SysMon
     * refresh is not worth it. A failure leaves the cache at zero and
     * is retried. */
    if (info->CPUFrequency == 0)
    {
        if (info->MaxCPUFrequency == 0)
            info->MaxCPUFrequency = vcQueryClock(ProcessorBase, VCTAG_GETCLKMAX, VCCLOCK_ARM);

        /* Fall back to the setpoint on firmware without the measured tag. */
        if ((info->CPUFrequency = vcQueryClock(ProcessorBase, VCTAG_GETCLKRATE_MEASURED, VCCLOCK_ARM)) == 0)
            info->CPUFrequency = vcQueryClock(ProcessorBase, VCTAG_GETCLKRATE, VCCLOCK_ARM);

        if (info->CPUFrequency == 0)
            info->CPUFrequency = info->MaxCPUFrequency;
    }

    return info->CPUFrequency;
}
