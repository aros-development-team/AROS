/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: owncard.c $

    Desc: OwnCard() function.
    Lang: english
*/

#include <proto/exec.h>

#include "card_intern.h"

AROS_LH1(struct CardHandle*, OwnCard,
	 AROS_LHA(struct CardHandle*, handle, A1),
	 struct CardResource*, CardResource, 1, Cardres)
{
    AROS_LIBFUNC_INIT
    
    struct CardHandle *ret = NULL;

    CARDDEBUG(bug("OwnCard(%p,%08x)\n", handle, handle->cah_CardFlags));

    handle->cah_CardFlags &= ~CARDF_USED;
    if (handle->cah_CardFlags & CARDF_DELAYOWNERSHIP) {
    	Forbid();
    	Enqueue(&CardResource->handles, &handle->cah_CardNode);
    	Permit();
	pcmcia_newowner(CardResource, TRUE);
	ret = (struct CardHandle*)-1;
    } else if (handle->cah_CardFlags & CARDF_IFAVAILABLE) {
    	if (CardResource->removed)
    	    ret = (struct CardHandle*)-1;
    	else if (CardResource->ownedcard)
    	    ret = CardResource->ownedcard;
    }

    if (ret == NULL) {
    	if (CardResource->removed)
	    ret = (struct CardHandle*)-1;
	else if (CardResource->ownedcard == NULL) {
	    Forbid();
	    AddHead(&CardResource->handles, &handle->cah_CardNode);
	    Permit();
	    pcmcia_newowner(CardResource, FALSE);
	} else
	    ret = 0;
    }
    	
    CARDDEBUG(bug("=%p\n", ret));

    return ret;

    AROS_LIBFUNC_EXIT
}
