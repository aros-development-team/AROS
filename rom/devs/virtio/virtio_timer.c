/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/exec.h>

#define __NOLIBBASE__
#include <proto/timer.h>

#include <exec/types.h>
#include <devices/timer.h>
#include <exec/io.h>

#include "virtio_intern.h"
#include "virtio_timer.h"

struct IORequest *virtio_OpenTimer(struct VirtIOBase *base)
{
    struct MsgPort *p = CreateMsgPort();
    if (NULL != p) {
        struct IORequest *io = CreateIORequest(p, sizeof(struct timerequest));

        if (NULL != io) {
            if (0 == OpenDevice("timer.device", UNIT_MICROHZ, io, 0)) {
                return io;
            } else {
                bug("[VIRTIO:Timer] Failed to open timer.device, unit MICROHZ\n");
            }
            DeleteIORequest(io);
        } else {
            bug("[VIRTIO:Timer] Failed to create timerequest\n");
        }
        DeleteMsgPort(p);
    } else {
        bug("[VIRTIO:Timer] Failed to create timer port\n");
    }

    return NULL;
}

void virtio_CloseTimer(struct IORequest *tmr)
{
    if (NULL != tmr) {
        struct MsgPort *p = tmr->io_Message.mn_ReplyPort;
        CloseDevice(tmr);
        DeleteIORequest(tmr);
        DeleteMsgPort(p);
    }
}

ULONG virtio_WaitTO(struct IORequest *tmr, ULONG secs, ULONG micro, ULONG sigs)
{
    ULONG tmrsig = 1 << tmr->io_Message.mn_ReplyPort->mp_SigBit;

    tmr->io_Command = TR_ADDREQUEST;
    tmr->io_Flags   = 0;
    ((struct timerequest *)tmr)->tr_time.tv_secs  = secs;
    ((struct timerequest *)tmr)->tr_time.tv_micro = micro;

    SendIO(tmr);
    sigs = Wait(sigs | tmrsig);
    if (0 == (sigs & tmrsig)) {
        if (!CheckIO(tmr))
            AbortIO(tmr);
    }
    WaitIO(tmr);

    SetSignal(0, tmrsig);

    return sigs & ~tmrsig;
}