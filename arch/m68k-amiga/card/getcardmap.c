/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.

    Desc: GetCardMap() function.
*/

#include "card_intern.h"

const struct CardMemoryMap cmm =
{
    (UBYTE*)GAYLE_RAM,
    (UBYTE*)GAYLE_ATTRIBUTE,
    (UBYTE*)GAYLE_IO,
    GAYLE_RAMSIZE,
    GAYLE_ATTRIBUTESIZE,
    GAYLE_IOSIZE
};

/*****************************************************************************
    NAME
        struct CardMemoryMap * GetCardMap(

    SYNOPSIS
        )

    FUNCTION
        Return the fixed m68k-Amiga Gayle PCMCIA address windows.

    RESULT
        A borrowed pointer to an immutable CardMemoryMap: common memory at
        0x00600000, size 0x00400000; attribute memory at 0x00A00000, size
        0x00020000; I/O memory at 0x00A20000, size 0x00010000.

    NOTES
        Do not modify or free the map. It remains valid for the lifetime of
        card.resource. Returning the map does not acquire a card or
        establish its presence. Claim the card before accessing its windows
        and stop access on removal. Attribute bytes occupy alternate
        addresses.

*****************************************************************************/
AROS_LH0(struct CardMemoryMap*, GetCardMap,
        struct CardResource*, CardResource, 3, Card)
{
    AROS_LIBFUNC_INIT

    CARDDEBUG(bug("GetCardMap()\n"));

    return (struct CardMemoryMap*)&cmm;

    AROS_LIBFUNC_EXIT
}
