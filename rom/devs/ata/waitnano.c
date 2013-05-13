/*
    Copyright � 2013, The AROS Development Team. All rights reserved
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

BOOL ata_Calibrate(struct IORequest* tmr, struct ataBase *base)
{
    register ULONG x;
    register ULONG scale = 0x8000;	// min iterations...
    volatile register ULONG t = 1;
    struct timeval t1, t2;
    struct Device *TimerBase = tmr->io_Device;
    
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
