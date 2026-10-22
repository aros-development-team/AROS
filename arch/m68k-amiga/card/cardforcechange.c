/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.

    Desc: CardForceChange() function.
*/

#include <proto/exec.h>

#include "card_intern.h"

/*****************************************************************************
    NAME
        BOOL CardForceChange(

    SYNOPSIS
        )

    FUNCTION
        Initiate a simulated m68k-Amiga card removal and, if still present,
        reinsertion.

    RESULT
        FALSE if removal-reset mode is enabled or the card is already marked
        removed. Otherwise TRUE after initiating the change.

    NOTES
        Call from task context. The function resets card configuration,
        marks the card removed and invokes the current owner's removal
        callback with task switching forbidden. The callback must arrange to
        release ownership.

        If hardware still reports a card, a synchronous card-reset sequence
        runs and the resource task is signalled to process reinsertion.
        Otherwise interrupts are re-enabled without signalling that task.
        TRUE does not mean reinsertion, ownership reassignment or a counter
        update has completed. The synchronous reset busy-waits on the
        display beam without a timeout.

*****************************************************************************/
AROS_LH0(ULONG, CardForceChange,
        struct CardResource*, CardResource, 15, Card)
{
    AROS_LIBFUNC_INIT

    CARDDEBUG(bug("CardForceChange()\n"));

    if (CardResource->resetberr & GAYLE_IRQ_RESET)
        return FALSE;
    if (CardResource->removed)
        return FALSE;

    pcmcia_reset(CardResource);
    CardResource->removed = TRUE;
    pcmcia_removeowner(CardResource);

    if (pcmcia_havecard()) {
        /* Simulate re-insertion of current card */
        CardResource->disabled = TRUE;
        pcmcia_cardreset(CardResource);
        pcmcia_enable_interrupts();
        Signal(CardResource->task, CardResource->signalmask);
    } else {
        pcmcia_enable_interrupts();
    }
        
    return TRUE;

    AROS_LIBFUNC_EXIT
}
