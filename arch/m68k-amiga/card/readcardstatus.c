/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.

    Desc: ReadCardStatus() function.
*/

#include "card_intern.h"

/*****************************************************************************
    NAME
        UBYTE ReadCardStatus(

    SYNOPSIS
        )

    FUNCTION
        Read the m68k-Amiga Gayle card-status signals.

    RESULT
        A mask containing CARD_STATUSF_CCDET, CARD_STATUSF_BVD1,
        CARD_STATUSF_BVD2, CARD_STATUSF_WR and CARD_STATUSF_BSY as read from
        the status register. Other bits are cleared.

    NOTES
        BVD1, BVD2 and BSY share their bit positions with SC, DA and IRQ for
        I/O cards. Interpret the signals for the card type. The call does
        not clear pending interrupts or check card ownership. A returned
        snapshot does not reserve access.

*****************************************************************************/
AROS_LH0(UBYTE, ReadCardStatus,
         struct CardResource*, CardResource, 6, Card)
{
    AROS_LIBFUNC_INIT

    volatile struct GayleIO *gio = (struct GayleIO*)GAYLE_BASE;
    return gio->status & (GAYLE_CS_CCDET | GAYLE_CS_BVD1 | GAYLE_CS_BVD2 | GAYLE_CS_WR | GAYLE_CS_BSY);

    AROS_LIBFUNC_EXIT
}
