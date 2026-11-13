/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.

    Desc: CardInterface() function.
*/

#include "card_intern.h"

/*****************************************************************************
    NAME
        ULONG CardInterface(

    SYNOPSIS
        )

    FUNCTION
        Identify the m68k-Amiga Gayle PCMCIA interface.

    RESULT
        CARD_INTERFACE_AMIGA_0 (zero).

    NOTES
        The identifier describes the resource implementation. It does not
        report card presence, ownership or readiness.

*****************************************************************************/
AROS_LH0(ULONG, CardInterface,
        struct CardResource*, CardResource, 17, Card)
{
    AROS_LIBFUNC_INIT

    return CARD_INTERFACE_AMIGA_0;

    AROS_LIBFUNC_EXIT
}
