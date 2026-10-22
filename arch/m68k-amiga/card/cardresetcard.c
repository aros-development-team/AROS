/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.

    Desc: CardResetCard() function.
*/

#include <proto/exec.h>

#include "card_intern.h"

/*****************************************************************************
    NAME
        BOOL CardResetCard(

    SYNOPSIS
        struct CardHandle *handle)

    FUNCTION
        Pulse the m68k-Amiga card-reset controls.

    INPUTS
        handle - Current card owner.

    RESULT
        TRUE after the reset sequence; FALSE if the ownership/presence check
        fails.

    NOTES
        Requires a non-NULL handle that currently owns the inserted card.
        The ownership test does not protect against subsequent removal.
        Call from task context. The sequence busy-waits for two groups of
        ten display-beam cycles and temporarily disables Gayle interrupt
        enables. It restores the saved enables and removal-reset setting. No
        card-ready status is checked and no timeout is provided; TRUE does
        not establish that the card is operational.

*****************************************************************************/
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
