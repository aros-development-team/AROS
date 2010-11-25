
#define DEBUG 0

#include <aros/debug.h>
#include <aros/libcall.h>

#include "battclock_intern.h"

UBYTE getreg(volatile UBYTE *p, UBYTE regnum)
{
    return p[regnum * 4 + 3] & 15;
}
void putreg(volatile UBYTE *p, UBYTE regnum, UBYTE v)
{
    p[regnum * 4 + 3] = v;
}
UBYTE getbcd(volatile UBYTE *p, UBYTE regnum)
{
    return getreg(p, regnum + 1) * 10 + getreg(p, regnum);
}

void resetbattclock(struct BattClockBase *Battclock)
{
    volatile UBYTE *p = Battclock->clockptr;
    UBYTE i, j;

    if (!p)
    	return;

    if (Battclock->clocktype == MSM6242B) {
    	putreg(p, 0xd, 0);
    	putreg(p, 0xe, 0);
    	putreg(p, 0xf, 2);
	for (i = 0; i < 12; i++)
	    putreg(p, i, 0);
	putreg(p, 0xf, 7); // reset
    	putreg(p, 0xf, 4); // leave 24h on
    } else if (Battclock->clocktype == RF5C01A) {
    	putreg(p, 0xd, 0); // stop
    	putreg(p, 0xe, 0);
    	for (j = 0; j < 4; j++) {
    	    putreg(p, 0xd, j);
    	    for (i = 0; i < 12; i++)
    	    	putreg(p, i, 0);
    	}
    	putreg(p, 0xd, 0);
    	putreg(p, 0xf, 3); // reset
    	putreg(p, 0xf, 0); // reset off
    	putreg(p, 0xd, 8); // timer en
    }
}