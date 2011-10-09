/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: cardprogramvoltage.c $

    Desc: CardProgramVoltage() function.
    Lang: english
*/

#include "card_intern.h"

AROS_LH2(LONG, CardProgramVoltage,
	AROS_LHA(struct CardHandle*, handle, A1),
	AROS_LHA(ULONG, voltage, D0),
	struct CardResource*, CardResource, 10, Cardres)
{
    AROS_LIBFUNC_INIT

    /* TODO but nobody probably cares */
    return -1;

    AROS_LIBFUNC_EXIT
}
