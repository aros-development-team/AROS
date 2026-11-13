/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.

    Desc: CardResetRemove() function.
*/

#include "card_intern.h"

/*****************************************************************************
    NAME
        BOOL CardResetRemove(

    SYNOPSIS
        struct CardHandle *handle, ULONG flag)

    FUNCTION
        Enable or disable machine reset on m68k-Amiga card-detect changes.

    INPUTS
        handle - Current card owner.
        flag - Zero disables reset; any nonzero value enables it.

    RESULT
        TRUE after applying the setting; FALSE if the ownership/presence
        check fails.

    NOTES
        Requires a non-NULL handle that currently owns the inserted card.
        The ownership test does not protect against subsequent removal.
        Enabling this setting permits a card-detect change to reset the
        machine. It is not a request to reset only the card. The bus-error
        control bit is cleared when the setting is written.

*****************************************************************************/
AROS_LH2(ULONG, CardResetRemove,
        AROS_LHA(struct CardHandle*, handle, A1),
        AROS_LHA(ULONG, flag, D0),
        struct CardResource*, CardResource, 7, Card)
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
