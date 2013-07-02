/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BeginCardAccess() function.
    Lang: english
*/

#include "card_intern.h"

AROS_LH1(ULONG, BeginCardAccess,
	AROS_LHA(struct CardHandle*, handle, A1),
	struct CardResource*, CardResource, 4, Cardres)
{
    AROS_LIBFUNC_INIT

    CARDDEBUG(bug("BeginCardAccess(%p)\n", handle));

    if (!ISMINE)
    	return FALSE;
    return TRUE;

    AROS_LIBFUNC_EXIT
}
