/*
    Copyright © 2009-2013, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/

#include <devices/timer.h>
#include <exec/io.h>
#include <proto/exec.h>
#include <aros/debug.h>
#include <proto/timer.h>

#include "timer.h"

struct IORequest *ata_OpenTimer(void)
{
    struct MsgPort *p = CreateMsgPort();

    if (NULL != p)
    {
        struct IORequest *io = CreateIORequest(p, sizeof(struct timerequest));

        if (NULL != io)
        {
            if (0 == OpenDevice("timer.device", UNIT_MICROHZ, io, 0))   
            {
                return io;
            }
            DeleteIORequest(io);
        }
        DeleteMsgPort(p);
    }

    return NULL;
}

void ata_CloseTimer(struct IORequest *tmr)
{
    if (NULL != tmr)
    {
        struct MsgPort *p = tmr->io_Message.mn_ReplyPort;

        CloseDevice(tmr);
        DeleteIORequest(tmr);
        DeleteMsgPort(p);
    }
}

void ata_WaitTO(struct IORequest* tmr, ULONG secs, ULONG micro)
{
    tmr->io_Command = TR_ADDREQUEST;
    ((struct timerequest*)tmr)->tr_time.tv_secs = secs;
    ((struct timerequest*)tmr)->tr_time.tv_micro = micro;

    DoIO(tmr);
}
