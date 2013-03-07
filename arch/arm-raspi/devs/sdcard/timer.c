/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <devices/timer.h>
#include <exec/io.h>
#include <proto/exec.h>
#include <aros/debug.h>
#include <proto/timer.h>

#include "timer.h"
#include "sdcard_intern.h"

struct IORequest *sdcard_OpenTimer(struct SDCardBase *SDCardBase)
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
		if (NULL == SDCardBase->sdcard_TimerBase)
		{
		    SDCardBase->sdcard_TimerBase = io->io_Device;
		}
		return io;
	    }
	    else
	    {
		bug("[SDCard  ] Failed to open timer.device, unit MICROHZ\n");
	    }
	    DeleteIORequest(io);
	}
	else
	{
	    bug("[SDCard  ] Failed to create timerequest\n");
	}
	DeleteMsgPort(p);
    }
    else
    {
	bug("[SDCard  ] Failed to create timer port\n");
    }

    return NULL;
}

void sdcard_CloseTimer(struct IORequest *tmr)
{
    if (NULL != tmr)
    {
	struct MsgPort *p = tmr->io_Message.mn_ReplyPort;
	CloseDevice(tmr);
	DeleteIORequest(tmr);
	DeleteMsgPort(p);
    }
}

void sdcard_WaitNano(register ULONG ns, struct SDCardBase *SDCardBase)
{
    volatile register ULONG t = 1;
    while (ns > 0)
    {
	asm volatile("mov r0, r0\n");
	--ns;
    }
}

ULONG sdcard_WaitTO(struct IORequest* tmr, ULONG secs, ULONG micro, ULONG sigs)
{
    ULONG sig = 1 << tmr->io_Message.mn_ReplyPort->mp_SigBit;

    //D(bug("[SDCard--] Timed wait %lds %ldu\n", secs, micro));

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

