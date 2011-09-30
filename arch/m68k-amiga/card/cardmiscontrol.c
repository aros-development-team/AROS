/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: cardmiscontrol.c $

    Desc: CardMiscControl() function.
    Lang: english
*/

#include <proto/exec.h>

#include "card_intern.h"

#define ALLOWMASK (GAYLE_INT_BVD1 | GAYLE_INT_BVD2 | GAYLE_INT_WR | GAYLE_INT_BSY)

AROS_LH2(UBYTE, CardMiscControl,
	AROS_LHA(struct CardHandle*, handle, A1),
	AROS_LHA(UBYTE, control_bits, D0),
	struct CardResource*, CardResource, 8, Cardres)
{
    AROS_LIBFUNC_INIT

    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;
    UBYTE control, val;

    CARDDEBUG(bug("CardMiscControl(%p,%02x)\n", handle, control_bits));

    if (!ISMINE)
    	return 0;

    val = control_bits & ALLOWMASK;
    
    Disable();

    control = gio->intena;

    if (control_bits & CARD_ENABLEF_DIGAUDIO)
    	control |= GAYLE_INT_DA;
    else
    	control &= ~GAYLE_INT_DA;

    if (control_bits & CARD_DISABLEF_WP)
    	control &= ~GAYLE_INT_WR;
    else
    	control |= GAYLE_INT_WR;

    if (control_bits & CARD_INTF_SETCLR)
    	control |= val;
    else
    	control &= ~val;

    gio->intena = control;

    Enable();    	

    return control_bits;

    AROS_LIBFUNC_EXIT
}
