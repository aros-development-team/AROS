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
    ns /= 2;
    if (!(SysBase->AttnFlags & AFF_68020))
    	ns /= 2;
    while (ns >= 65536 * 4) {
        busywait(65535);
        ns -= 65536 * 4;
    }
    if (ns >= 4)
        busywait(ns / 4);
}
