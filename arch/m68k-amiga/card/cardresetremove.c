/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CardResetRemove() function.
    Lang: english
*/

#include "card_intern.h"

AROS_LH2(ULONG, CardResetRemove,
	AROS_LHA(struct CardHandle*, handle, A1),
	AROS_LHA(ULONG, flag, D0),
	struct CardResource*, CardResource, 7, Cardres)
{
    AROS_LIBFUNC_INIT

    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;

    CARDDEBUG(bug("CardResetRemove(%p,%08x)\n", handle, flag));

    if (!ISMINE)
    	return 0;

    CardResource->resetberr = flag ? GAYLE_IRQ_RESET : 0;
    gio->intreq = (0xff & ~(GAYLE_IRQ_RESET | GAYLE_IRQ_BERR)) | CardResource->resetberr;

    return 1;

    AROS_LIBFUNC_EXIT
}
