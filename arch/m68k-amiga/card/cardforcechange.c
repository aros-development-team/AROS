/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id: cardforcechange.c $

    Desc: CardForceChange() function.
    Lang: english
*/

#include <proto/exec.h>

#include "card_intern.h"

AROS_LH0(ULONG, CardForceChange,
	struct CardResource*, CardResource, 15, Cardres)
{
    AROS_LIBFUNC_INIT

    struct CardHandle *cah = CardResource->ownedcard;

    CARDDEBUG(bug("CardForceChange()\n"));

    if (CardResource->resetberr & GAYLE_IRQ_RESET)
    	return FALSE;

    pcmcia_reset(CardResource);
    CardResource->removed = TRUE;
    pcmcia_removeowner(CardResource);

    if (pcmcia_havecard(CardResource)) {
    	/* Simulate re-insertion of current card */
    	CardResource->disabled = TRUE;
    	if (cah != NULL)
	    Enqueue(&CardResource->handles, &cah->cah_CardNode);
    	pcmcia_cardreset(CardResource);
    	pcmcia_enable_interrupts(CardResource);
    	Signal(CardResource->task, CardResource->signalmask);
    } else {
    	pcmcia_enable_interrupts(CardResource);
    }
    	
    return TRUE;

    AROS_LIBFUNC_EXIT
}
