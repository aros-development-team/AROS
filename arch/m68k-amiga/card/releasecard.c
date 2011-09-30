/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: releasecard.c $

    Desc: ReleaseCard() function.
    Lang: english
*/

#include <proto/exec.h>

#include "card_intern.h"

AROS_LH2(void, ReleaseCard,
	 AROS_LHA(struct CardHandle*, handle, A1),
	 AROS_LHA(ULONG, flags, D0),
	 struct CardResource*, CardResource, 2, Cardres)
{
    AROS_LIBFUNC_INIT

    CARDDEBUG(bug("ReleaseCard(%p)\n", handle));

    Disable();

    if (CardResource->ownedcard == handle) {
	CardResource->ownedcard = NULL;
	pcmcia_reset(CardResource);
    } else if (flags & CARDF_REMOVEHANDLE) {
    	Remove(&handle->cah_CardNode);
    }

    Enable();

    AROS_LIBFUNC_EXIT
}
