/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved

    Desc:
*/

#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/timer.h>

#include <exec/types.h>
#include <devices/timer.h>
#include <exec/io.h>

#include "nvme_intern.h"
#include "nvme_timer.h"

struct IORequest *nvme_OpenTimer(struct NVMEBase *base)
{
    struct MsgPort *p = CreateMsgPort();
    if (NULL != p)
    {
        struct IORequest *io = CreateIORequest(p, sizeof(struct timerequest));

        if (NULL != io)
        {
            /*
             * ok. ECLOCK does not have too great resolution, either.
             * we will have to sacrifice our performance a little bit, meaning, the 400ns will turn into (worst case) 2us.
             * hopefully we won't have to call that TOO often...
             */
            if (0 == OpenDevice("timer.device", UNIT_MICROHZ, io, 0))
            {
                return io;
            }
            else
            {
                bug("[NVME:Timer] Failed to open timer.device, unit MICROHZ\n");
            }
            DeleteIORequest(io);
        }
        else
        {
            bug("[NVME:Timer] Failed to create timerequest\n");
        }
        DeleteMsgPort(p);
    }
    else
    {
        bug("[NVME:Timer] Failed to create timer port\n");
    }

    return NULL;
}

void nvme_CloseTimer(struct IORequest *tmr)
{
    if (NULL != tmr)
    {
        struct MsgPort *p = tmr->io_Message.mn_ReplyPort;
        CloseDevice(tmr);
        DeleteIORequest(tmr);
        DeleteMsgPort(p);
    }
}


ULONG nvme_WaitTO(struct IORequest* tmr, ULONG secs, ULONG micro, ULONG sigs)
{
    ULONG tmrsig = 1 << tmr->io_Message.mn_ReplyPort->mp_SigBit;

    tmr->io_Command = TR_ADDREQUEST;
    tmr->io_Flags   = 0;
    ((struct timerequest*)tmr)->tr_time.tv_secs = secs;
    ((struct timerequest*)tmr)->tr_time.tv_micro = micro;

    SendIO(tmr);
    sigs = Wait(sigs | tmrsig);
    if (0 == (sigs & tmrsig)) {
        if (!CheckIO(tmr))
            AbortIO(tmr);
    }
    WaitIO(tmr);

    SetSignal(0, tmrsig);

    return sigs &~ tmrsig;
}
