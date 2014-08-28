/*
    Copyright � 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GetCardMap() function.
    Lang: english
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

AROS_LH0(struct CardMemoryMap*, GetCardMap,
	struct CardResource*, CardResource, 3, Card)
{
    AROS_LIBFUNC_INIT

    CARDDEBUG(bug("GetCardMap()\n"));

    return (struct CardMemoryMap*)&cmm;

    AROS_LIBFUNC_EXIT
}
