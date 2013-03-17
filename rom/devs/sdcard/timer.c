/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <devices/timer.h>
#include <exec/io.h>
#include <proto/exec.h>
#include <aros/debug.h>
#include <proto/timer.h>

#include "timer.h"
#include "sdcard_base.h"

#include LC_LIBDEFS_FILE

struct IORequest *sdcard_OpenTimer(LIBBASETYPEPTR LIBBASE)
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
		if (NULL == LIBBASE->sdcard_TimerBase)
		{
		    LIBBASE->sdcard_TimerBase = io->io_Device;
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

ULONG sdcard_WaitTO(struct IORequest* tmr, ULONG secs, ULONG micro, ULONG sigs)
{
    ULONG sig = 1 << tmr->io_Message.mn_ReplyPort->mp_SigBit;

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

