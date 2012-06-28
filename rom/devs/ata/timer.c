/*
    Copyright © 2009, The AROS Development Team. All rights reserved
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

#include <exec/types.h>
#include <devices/timer.h>
#include <exec/io.h>
#include <proto/exec.h>
#include <aros/debug.h>
#include <proto/timer.h>

#include "timer.h"
#include "ata.h"

static BOOL ata_Calibrate(struct IORequest* tmr, struct ataBase *base)
{
    register ULONG x;
    register ULONG scale = 0x8000;	// min iterations...
    volatile register ULONG t = 1;
    struct timeval t1, t2;
    struct Device *TimerBase = base->ata_TimerBase;
    
    D(bug("[ATA  ] Calibration started\n"));

    while (scale <= 0x80000000)
    {
	Forbid();
	GetSysTime(&t1);
	for (x = 1; x < scale; x++)
	    t = (((t + x) * t) - x) / x;    // add, mul, sub, div, trivial benchmark.

	GetSysTime(&t2);
	Permit();
	SubTime(&t2, &t1);
	
	// ok, it's going to be totally insane, if secs > 1.
	if (t2.tv_secs != 0)
	{
	    bug("[ATA  ] micro wait useless.\n");
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

    D(bug("[ATA  ] Executed %ld ops in %ldus\n", scale, t2.tv_micro));

    // always round up to the next value.. so 30.9 -> 31, 5.1 -> 6, etc 
    x = (x + t2.tv_micro - 1) / t2.tv_micro;
    x = (x+9) / 10;

    bug("[ATA  ] Approximate number of iterations per 100 nanoseconds: %ld\n", x);
    base->ata_ItersPer100ns = x;
    return TRUE;
}

struct IORequest *ata_OpenTimer(struct ataBase *base)
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
		if (NULL == base->ata_TimerBase)
		{
		    base->ata_TimerBase = io->io_Device;
		    ata_Calibrate(io, base);
		}
		return io;
	    }
	    else
	    {
		bug("[ATA  ] Failed to open timer.device, unit MICROHZ\n");
	    }
	    DeleteIORequest(io);
	}
	else
	{
	    bug("[ATA  ] Failed to create timerequest\n");
	}
	DeleteMsgPort(p);
    }
    else
    {
	bug("[ATA  ] Failed to create timer port\n");
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

void ata_WaitNano(register ULONG ns, struct ataBase *base)
{
    volatile register ULONG t = 1;
    ns = (ns + 99) / 100;
    ns *= base->ata_ItersPer100ns;
    while (ns > 0)
    {
	t = (((t + ns) * t) - ns) / ns;    // add, mul, sub, div, trivial benchmark.
	--ns;
    }
}

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

