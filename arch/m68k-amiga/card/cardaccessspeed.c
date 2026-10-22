/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.

    Desc: CardAccessSpeed() function.
*/

#include "card_intern.h"

/*****************************************************************************
    NAME
        ULONG CardAccessSpeed(

    SYNOPSIS
        struct CardHandle *handle, ULONG nanoseconds)

    FUNCTION
        Set the m68k-Amiga card-memory access timing.

    INPUTS
        handle - Current card owner.
        nanoseconds - Requested access time. Values 0-100 select 100 ns;
        101-150 select 150 ns; 151-250 select 250 ns; 251-720 select 720 ns.

    RESULT
        The selected access time in nanoseconds, or zero if the request
        exceeds 720 ns or the ownership/presence check fails.

    NOTES
        Requires a non-NULL handle that currently owns the inserted card.
        The ownership test does not protect against subsequent removal.
        Rejected requests leave the configuration unchanged. The
        programming-voltage bits are preserved. A zero request selects 100
        ns; it does not query the current timing.

*****************************************************************************/
AROS_LH2(ULONG, CardAccessSpeed,
        AROS_LHA(struct CardHandle*, handle, A1),
        AROS_LHA(ULONG, nanoseconds, D0),
        struct CardResource*, CardResource, 9, Card)
{
    AROS_LIBFUNC_INIT

    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;
    UBYTE v, speed;

    CARDDEBUG(bug("CardAccessSpeed(%p,%d)\n", handle, nanoseconds));

    if (!ISMINE)
        return 0;

    speed = 0;

    if (nanoseconds > 720)
        return 0;

    if (nanoseconds > 250) {
        speed = GAYLE_CFG_720NS;
        nanoseconds = 720;
    } else if (nanoseconds > 150) {
        speed = GAYLE_CFG_250NS;
        nanoseconds = 250;
    } else if (nanoseconds > 100) {
        speed = GAYLE_CFG_150NS;
        nanoseconds = 150;
    } else {
        speed = GAYLE_CFG_100NS;
        nanoseconds = 100;
    }
    
    Disable();
    v = gio->config & ~GAYLE_CFG_SPEED;
    v |= speed;
    gio->config = v;
    Enable();
    
    return nanoseconds;

    AROS_LIBFUNC_EXIT
}
