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

/* Single CIA access = 1 E-clock */
void ata_WaitNano(register ULONG ns, struct ataBase *base)
{
    volatile struct CIA *cia = (struct CIA*)0xbfe001;
    for (;;) {
        cia->ciapra; /* dummy CIA read */
        if (ns < 700)
            break;
        ns -= 700;
    }
}
