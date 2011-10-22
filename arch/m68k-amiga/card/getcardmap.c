/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: getcardmap.c $

    Desc: GetCardMap() function.
    Lang: english
*/

#include "card_intern.h"

AROS_LH0(struct CardMemoryMap*, GetCardMap,
	struct CardResource*, CardResource, 3, Cardres)
{
    AROS_LIBFUNC_INIT

    CARDDEBUG(bug("GetCardMap()\n"));

    return &CardResource->cmm;

    AROS_LIBFUNC_EXIT
}
