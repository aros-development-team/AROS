/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CardMiscControl() function.
    Lang: english
*/

#include <proto/exec.h>

#include "card_intern.h"

#define STATUSMASK (GAYLE_CS_WR | GAYLE_CS_DAEN)
#define IRQMASK (GAYLE_INT_BVD1 | GAYLE_INT_BVD2 | GAYLE_INT_BSY)

AROS_LH2(UBYTE, CardMiscControl,
	AROS_LHA(struct CardHandle*, handle, A1),
	AROS_LHA(UBYTE, control_bits, D1),
	struct CardResource*, CardResource, 8, Card)
{
    AROS_LIBFUNC_INIT

    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;
    UBYTE control, val;

    CARDDEBUG(bug("CardMiscControl(%p,%02x)\n", handle, control_bits));

    if (!ISMINE)
    	return 0;

    val = control_bits & IRQMASK;

    Disable();

    gio->status = control_bits & STATUSMASK;

    if (val) {
	control = gio->intena;
	if (control_bits & CARD_INTF_SETCLR)
	    control |= val;
	else
	    control &= ~val;
	gio->intena = control;
    }

    Enable();    	

    return control_bits & (STATUSMASK | IRQMASK | CARD_INTF_SETCLR);

    AROS_LIBFUNC_EXIT
}
