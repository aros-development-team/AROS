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

    CARDDEBUG(bug("ReleaseCard(%p,%08x) flags=%02x owned=%p removed=%d\n",
	handle, flags, handle->cah_CardFlags, CardResource->ownedcard, CardResource->removed));

    handle->cah_CardFlags &= ~CARDF_USED;
    if (CardResource->ownedcard == handle) {
    	if (CardResource->removed == FALSE)
	    handle->cah_CardFlags |= CARDF_USED;
	CardResource->ownedcard = NULL;
	pcmcia_reset(CardResource);
	pcmcia_enable_interrupts();

	if (CardResource->removed == FALSE)
	    pcmcia_newowner(CardResource);
    }

    if (flags & CARDF_REMOVEHANDLE) {
	Forbid();
	Remove(&handle->cah_CardNode);
	Permit();
	handle->cah_CardFlags &= ~CARDF_USED;
    }

    AROS_LIBFUNC_EXIT
}
