/*
    Copyright � 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CardInterface() function.
    Lang: english
*/

#include "card_intern.h"

AROS_LH0(ULONG, CardInterface,
	struct CardResource*, CardResource, 17, Card)
{
    AROS_LIBFUNC_INIT

    return CARD_INTERFACE_AMIGA_0;

    AROS_LIBFUNC_EXIT
}
