/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.

    Desc: CardMiscControl() function.
*/

#include <proto/exec.h>

#include "card_intern.h"

#define STATUSMASK (GAYLE_CS_WR | GAYLE_CS_DAEN)
#define IRQMASK (GAYLE_INT_BVD1 | GAYLE_INT_BVD2 | GAYLE_INT_BSY)

/*****************************************************************************
    NAME
        UBYTE CardMiscControl(

    SYNOPSIS
        struct CardHandle *handle, UBYTE control_bits)

    FUNCTION
        Update m68k-Amiga card status controls and selected interrupt
        enables.

    INPUTS
        handle - Current card owner.
        control_bits - CARD_ENABLEF_DIGAUDIO and CARD_DISABLEF_WP replace
        the corresponding status controls. CARD_INTF_BVD1, CARD_INTF_BVD2
        and CARD_INTF_BSY select interrupt enables; CARD_INTF_SETCLR sets
        the selected bits when present and clears them otherwise. SC, DA and
        IRQ are aliases for those interrupt bits.

    RESULT
        The requested bits masked to the supported controls, including
        CARD_INTF_SETCLR, or zero if the ownership/presence check fails.
        Zero is also a valid successful request.

    NOTES
        Requires a non-NULL handle that currently owns the inserted card.
        The ownership test does not protect against subsequent removal.
        Status controls are replaced on every call, including calls intended
        only to alter interrupts. Unselected interrupt enables are
        preserved. The result is the masked request, not the previous state
        or a hardware readback.

*****************************************************************************/
AROS_LH2(UBYTE, CardMiscControl,
        AROS_LHA(struct CardHandle*, handle, A1),
        AROS_LHA(UBYTE, control_bits, D1),
        struct CardResource*, CardResource, 8, Card)
{
    AROS_LIBFUNC_INIT

    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;
    UBYTE control, val;

    CARDDEBUG(bug("CardMiscControl(%p,%02x)\n", handle, control_bits));

    if (!ISMINE)
        return 0;

    val = control_bits & IRQMASK;

    Disable();

    gio->status = control_bits & STATUSMASK;

    if (val) {
        control = gio->intena;
        if (control_bits & CARD_INTF_SETCLR)
            control |= val;
        else
            control &= ~val;
        gio->intena = control;
    }

    Enable();

    return control_bits & (STATUSMASK | IRQMASK | CARD_INTF_SETCLR);

    AROS_LIBFUNC_EXIT
}
