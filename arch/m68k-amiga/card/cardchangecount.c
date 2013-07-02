/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CardChangeCount() function.
    Lang: english
*/

#include <proto/exec.h>

#include "card_intern.h"

AROS_LH0(ULONG, CardChangeCount,
	struct CardResource*, CardResource, 16, Cardres)
{
    AROS_LIBFUNC_INIT

    return CardResource->changecount;

    AROS_LIBFUNC_EXIT
}
