/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <hardware/custom.h>
#include <proto/exec.h>

#include "battclock_intern.h"

static int BattClock_Init(struct BattClockBase *BattClockBase)
{
    volatile UBYTE *p = (volatile UBYTE*)0xdc0000;
    volatile struct Custom *cm = (volatile struct Custom*)0xdc0000;
    volatile struct Custom *c = (volatile struct Custom*)0xdff000;
    BOOL found = TRUE, didreset = FALSE;
    BYTE rounds = 2;

    Disable();
    for (;;) {
    	UWORD adkcon = c->adkconr;
	c->adkcon = 0x7fff;
	if (c->adkconr == cm->adkconr) {
	    c->adkcon = 0x8001;
	    if (c->adkconr == cm->adkconr) {
	        c->adkcon = 0x0001;
	        if (c->adkconr == cm->adkconr)
	       	    found = FALSE;
	    }
	}
	c->adkcon = 0x7fff;
	c->adkcon = 0x8000 | adkcon;
	if (!found) {
	    D(bug("custom chipset detected in clock space\n"));
	    break;
	}
	// ok, not custom, check if we have a clock
	found = FALSE;
	// 10 minutes or hours bit 3 set = not a clock
	if ((getreg(p, 0x01) & 0x8) || (getreg(p, 0x03) & 8))
	    break;
	// this is not easy, most UAE versions emulate clock
	// registers very badly (read-only registers are read-write etc..)
	// so this needs to be quite stupid.
	while (rounds-- > 0) {
	    if (getreg(p, 0x0d) == 0x8 && getreg(p, 0x0f) == 0x0) {
	        // RF, maybe
 	        BattClockBase->clocktype = RF5C01A;
    		BattClockBase->clockptr = p;
    		found = TRUE;
 	        break;
	    } else if (getreg(p, 0x0f) == 0x4) {
	        // MSM
	        BattClockBase->clocktype = MSM6242B;
    		BattClockBase->clockptr = p;
    		found = TRUE;
	        break;
	    } else {
	        // reset clock
	        putreg(p, 0xe, 0);
	        putreg(p, 0xd, 0);
	        putreg(p, 0xd, 4); // set irq flag
	        if (getreg(p, 0xd) == 0) {
		    // was MSM, irq can't be set
	    	    putreg(p, 0xf, 7); // reset
	    	    putreg(p, 0xf, 4); // leave 24h on
	    	} else { // was alarm en
	    	    // RF
	    	    putreg(p, 0xf, 3); // reset
	    	    putreg(p, 0xf, 0); // reset off
	    	    putreg(p, 0xd, 8); // timer en
	    	}
	    	didreset = TRUE;
	    }
	    if (didreset)
	    	resetbattclock(BattClockBase);
	}
	break;
    }
    Enable();
    if (found)
    	BattClockBase->UtilityBase = (struct UtilityBase*)OpenLibrary("utility.library", 0);
    D(bug("BattClockBase init=%d clock=%d\n", found, BattClockBase->clocktype));
    return found;
}

ADD2INITLIB(BattClock_Init, 0)
