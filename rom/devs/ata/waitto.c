/*
    Copyright © 2013, The AROS Development Team. All rights reserved
    $Id$
*/

#include <exec/types.h>
#include <devices/timer.h>
#include <exec/io.h>
#include <proto/exec.h>
#include <aros/debug.h>
#include <proto/timer.h>

#include "timer.h"
#include "ata.h"

ULONG ata_WaitTO(struct IORequest* tmr, ULONG secs, ULONG micro, ULONG sigs)
{
    ULONG sig = 1 << tmr->io_Message.mn_ReplyPort->mp_SigBit;

    //D(bug("[ATA--] Timed wait %lds %ldu\n", secs, micro));

    tmr->io_Command = TR_ADDREQUEST;
    ((struct timerequest*)tmr)->tr_time.tv_secs = secs;
    ((struct timerequest*)tmr)->tr_time.tv_micro = micro;

    SendIO(tmr);
    sigs = Wait(sigs | sig);
    if (0 == (sigs & sig))
    {
	if (!CheckIO(tmr))
	    AbortIO(tmr);
    }
    WaitIO(tmr);

    SetSignal(0, sig);

    return sigs &~ sig;
}
