/*
    Copyright © 2009-2013, The AROS Development Team. All rights reserved
    $Id: timer.c 55802 2019-03-08 21:47:59Z wawa $

    Desc:
    Lang: English
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

struct IORequest *scsi_OpenTimer(struct scsiBase *base)
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
		bug("[SCSI  ] Failed to open timer.device, unit MICROHZ\n");
	    }
	    DeleteIORequest(io);
	}
	else
	{
	    bug("[SCSI  ] Failed to create timerequest\n");
	}
	DeleteMsgPort(p);
    }
    else
    {
	bug("[SCSI  ] Failed to create timer port\n");
    }

    return NULL;
}

void scsi_CloseTimer(struct IORequest *tmr)
{
    if (NULL != tmr)
    {
	struct MsgPort *p = tmr->io_Message.mn_ReplyPort;
	CloseDevice(tmr);
	DeleteIORequest(tmr);
	DeleteMsgPort(p);
    }
}
