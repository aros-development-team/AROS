/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.

    Desc: CardProgramVoltage() function.
*/

#include "card_intern.h"

/*****************************************************************************
    NAME
        LONG CardProgramVoltage(

    SYNOPSIS
        struct CardHandle *handle, ULONG voltage)

    FUNCTION
        Set the m68k-Amiga card programming voltage.

    INPUTS
        handle - Current card owner.
        voltage - CARD_VOLTAGE_0V or CARD_VOLTAGE_5V. These are selector
        constants, not a numeric voltage.

    RESULT
        1 after applying a supported selector; -1 for an unsupported
        selector, including CARD_VOLTAGE_12V; 0 if the ownership/presence
        check fails.

    NOTES
        Requires a non-NULL handle that currently owns the inserted card.
        The ownership test does not protect against subsequent removal.
        Rejected requests leave the configuration unchanged. Access-timing
        bits are preserved. Ownership failure takes precedence over selector
        validation.

*****************************************************************************/
AROS_LH2(LONG, CardProgramVoltage,
        AROS_LHA(struct CardHandle*, handle, A1),
        AROS_LHA(ULONG, voltage, D0),
        struct CardResource*, CardResource, 10, Card)
{
    AROS_LIBFUNC_INIT

    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;
    UBYTE v;

    CARDDEBUG(bug("CardProgramVoltage(%p,%d)\n", handle, voltage));

    if (!ISMINE)
        return 0;
        
    if (voltage != GAYLE_CFG_0V && voltage != GAYLE_CFG_5V)
        return -1; /* Don't want 12V at this point.. */
    
    Disable();
    v = gio->config & ~GAYLE_CFG_VOLTAGE;
    v |= voltage;
    gio->config = v;
    Enable();

    return 1;

    AROS_LIBFUNC_EXIT
}
