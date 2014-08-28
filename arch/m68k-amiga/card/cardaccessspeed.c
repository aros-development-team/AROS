/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CardAccessSpeed() function.
    Lang: english
*/

#include "card_intern.h"

AROS_LH2(ULONG, CardAccessSpeed,
	AROS_LHA(struct CardHandle*, handle, A1),
	AROS_LHA(ULONG, nanoseconds, D0),
	struct CardResource*, CardResource, 9, Card)
{
    AROS_LIBFUNC_INIT

    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;
    UBYTE v, speed;

    CARDDEBUG(bug("CardAccessSpeed(%p,%d)\n", handle, nanoseconds));

    if (!ISMINE)
    	return 0;

    speed = 0;

    if (nanoseconds > 720)
    	return 0;

    if (nanoseconds > 250) {
	speed = GAYLE_CFG_720NS;
	nanoseconds = 720;
    } else if (nanoseconds > 150) {
	speed = GAYLE_CFG_250NS;
	nanoseconds = 250;
    } else if (nanoseconds > 100) {
	speed = GAYLE_CFG_150NS;
	nanoseconds = 150;
    } else {
	speed = GAYLE_CFG_100NS;
	nanoseconds = 100;
    }
    
    Disable();
    v = gio->config & ~GAYLE_CFG_SPEED;
    v |= speed;
    gio->config = v;
    Enable();
    
    return nanoseconds;

    AROS_LIBFUNC_EXIT
}
