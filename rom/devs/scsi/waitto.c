/*
    Copyright © 2013-2014, The AROS Development Team. All rights reserved
    $Id: waitto.c 55802 2019-03-08 21:47:59Z wawa $
*/

#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/timer.h>

#include <exec/types.h>
#include <devices/timer.h>
#include <exec/io.h>

#include "timer.h"
#include "scsi.h"

/* Waits for a signal or a timeout */
ULONG scsi_WaitTO(struct IORequest* tmr, ULONG secs, ULONG micro, ULONG sigs)
{
    ULONG sig = 1 << tmr->io_Message.mn_ReplyPort->mp_SigBit;

    D(struct Node *t = (struct Node *)FindTask(NULL));
    D(bug("[SCSI  ] Timed wait %lds %ldu (task='%s')\n", secs, micro,
        t->ln_Name));

    tmr->io_Command = TR_ADDREQUEST;
    ((struct timerequest*)tmr)->tr_time.tv_secs = secs;
    ((struct timerequest*)tmr)->tr_time.tv_micro = micro;

    SendIO(tmr);
    D(bug("[SCSI  ] Preset signals: %lx ('%s')\n", SetSignal(0, 0), t->ln_Name));
    D(bug("[SCSI  ] Signals requested: %lx ('%s')\n", sigs, t->ln_Name));
    D(bug("[SCSI  ] Timer signal: %lx ('%s')\n", sig, t->ln_Name));
    sigs = Wait(sigs | sig);
    D(bug("[SCSI  ] Signals received: %lx ('%s')\n", sigs, t->ln_Name));
    if (0 == (sigs & sig))
    {
	if (!CheckIO(tmr))
	    AbortIO(tmr);
    }
    WaitIO(tmr);

    SetSignal(0, sig);

    return sigs & ~sig;
}
