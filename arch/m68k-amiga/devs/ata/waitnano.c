/*
    Copyright © 2013, The AROS Development Team. All rights reserved
    $Id$
*/

#include <exec/types.h>
#include <exec/io.h>
#include <hardware/cia.h>
#include <aros/debug.h>

#include "timer.h"
#include "ata.h"

BOOL ata_Calibrate(struct IORequest* tmr, struct ataBase *base)
{
    base->ata_ItersPer100ns = 1;
    return TRUE;
}

static void busywait(UWORD cnt)
{
    asm volatile (
    	"move.w %0,%%d0\n"
        "lea 0xbfe001,%%a0\n"
        "0:\n"
        "tst.b (%%a0)\n"
        "tst.b (%%a0)\n"
        "tst.b (%%a0)\n"
        "tst.b (%%a0)\n"
        "dbf %%d0,0b\n"
    : : "m" (cnt) : "d0", "a0");
}

/* Single CIA access = 1 E-clock */
void ata_WaitNano(ULONG ns, struct ataBase *base)
{
    /*
        We really need to review this code. The WaitNano is supposed to wait given number of *nanoseconds*
        Each CIA access takes one E-clock which equals 1400 nanoseconds. It means, on Amiga hardware it is
        hardly impossible to do a delay shorter than 1.4 microseconds.
        
        For how we will shift the nanosecond count by 10 bits, effectively dividing it by 1024.
    */
    ns /= 1024;
    if (!(SysBase->AttnFlags & AFF_68020))
    	ns /= 2;
    while (ns >= 65536 * 4) {
        busywait(65535);
        ns -= 65536 * 4;
    }
    if (ns >= 4)
        busywait(ns / 4);
}
