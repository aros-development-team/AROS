/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CardResetCard() function.
    Lang: english
*/

#include <proto/exec.h>

#include "card_intern.h"

AROS_LH1(ULONG, CardResetCard,
	AROS_LHA(struct CardHandle*, handle, A1),
	struct CardResource*, CardResource, 11, Card)
{
    AROS_LIBFUNC_INIT

    CARDDEBUG(bug("CardResetCard(%p)\n", handle));

    if (!ISMINE)
    	return FALSE;

    pcmcia_cardreset(CardResource);

    return TRUE;

    AROS_LIBFUNC_EXIT
}
