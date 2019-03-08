/*
    Copyright © 2009-2018, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/
/*
 * PARTIAL CHANGELOG:
 * DATE        NAME                ENTRY
 * ----------  ------------------  -------------------------------------------------------------------
 * 2005-03-06  T. Wiszkowski       few corrections (thanks, Georg)
 * 2005-03-05  T. Wiszkowski       created file; initial benchmarked nanowait and timer-based micro/sec wait
 */
 
#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <exec/types.h>
#include <devices/timer.h>
#include <exec/io.h>
#include <proto/timer.h>

#include "timer.h"

ULONG iters_per_100ns = ~0;

static BOOL ahci_Calibrate(struct IORequest* tmr)
{
    struct Device *TimerBase = tmr->io_Device;
    register ULONG x;
    register ULONG scale = 0x8000;	// min iterations...
    volatile register ULONG t = 1;
    struct timeval t1, t2;
    
    D(bug("[AHCI  ] Calibration started\n"));

    while (scale <= 0x80000000)
    {
	Forbid();
	GetUpTime(&t1);
	for (x = 1; x < scale; x++)
	    t = (((t + x) * t) - x) / x;    // add, mul, sub, div, trivial benchmark.

	GetUpTime(&t2);
	Permit();
	SubTime(&t2, &t1);
	
	// ok, it's going to be totally insane, if secs > 1.
	if (t2.tv_secs != 0)
	{
	    bug("[AHCI  ] micro wait useless.\n");
	    return FALSE;
	}

	/* 
	 * we expect at least 10000 times longer period, which should be 'achievable'
	 * unlikely we will cross the magic boundary here of 4 billion instructions in 10 millisecond (yielding 400'000MIPS?)
	 * on the other side, if we go as low as 1, then 4 iterations of add/sub/mul/div is perfectly fine yielding a bit more than 400ns...
	 */

	if (t2.tv_micro >= 10000)
	    break;
	scale <<= 1;
    }

    D(bug("[AHCI  ] Executed %ld ops in %ldus\n", scale, t2.tv_micro));

    // always round up to the next value.. so 30.9 -> 31, 5.1 -> 6, etc 
    x = (x + t2.tv_micro - 1) / t2.tv_micro;
    x = (x+9) / 10;

    bug("[AHCI  ] Approximate number of iterations per 100 nanoseconds: %ld\n", x);
    iters_per_100ns = x;
    return TRUE;
}

struct IORequest *ahci_OpenTimer()
{
    struct MsgPort *p = CreateMsgPort();
    if (NULL != p)
    {
	struct IORequest *io = CreateIORequest(p, sizeof(struct timerequest));

	if (NULL != io)
	{
	    /*
	     * We only need this timer for relatively long delays
	     */
	    if (0 == OpenDevice(TIMERNAME, UNIT_MICROHZ, io, 0))	
	    {
		if (iters_per_100ns == ~0)
		{
		    ahci_Calibrate(io);
		}
		return io;
	    }
	    else
	    {
		bug("[AHCI  ] Failed to open timer.device, unit MICROHZ\n");
	    }
	    DeleteIORequest(io);
	}
	else
	{
	    bug("[AHCI  ] Failed to create timerequest\n");
	}
	DeleteMsgPort(p);
    }
    else
    {
	bug("[AHCI  ] Failed to create timer port\n");
    }

    return NULL;
}

void ahci_CloseTimer(struct IORequest *tmr)
{
    if (NULL != tmr)
    {
	struct MsgPort *p = tmr->io_Message.mn_ReplyPort;
	CloseDevice(tmr);
	DeleteIORequest(tmr);
	DeleteMsgPort(p);
    }
}

void ahci_WaitNano(register ULONG ns)
{
    volatile register ULONG t = 1;
    ns = (ns + 99) / 100;
    ns *= iters_per_100ns;
    while (ns > 0)
    {
	t = (((t + ns) * t) - ns) / ns;    // add, mul, sub, div, trivial benchmark.
	--ns;
    }
}

ULONG ahci_WaitTO(struct IORequest* tmr, ULONG secs, ULONG micro, ULONG sigs)
{
    ULONG iosig = 1 << tmr->io_Message.mn_ReplyPort->mp_SigBit;

    //D(bug("[AHCI--] Timed wait %lds %ldu\n", secs, micro));

    tmr->io_Command = TR_ADDREQUEST;
    tmr->io_Flags   = 0;
    ((struct timerequest*)tmr)->tr_time.tv_secs = secs;
    ((struct timerequest*)tmr)->tr_time.tv_micro = micro;

    SendIO(tmr);
    sigs = Wait(sigs | iosig);
    if (0 == (sigs & iosig)) {
	if (!CheckIO(tmr))
	    AbortIO(tmr);
    }
    WaitIO(tmr);

    SetSignal(0, iosig);

    return sigs &~ iosig;
}

