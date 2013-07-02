/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: EndCardAccess() function.
    Lang: english
*/

#include "card_intern.h"

AROS_LH1(ULONG, EndCardAccess,
	AROS_LHA(struct CardHandle*, handle, A1),
	struct CardResource*, CardResource, 5, Cardres)
{
    AROS_LIBFUNC_INIT

    CARDDEBUG(bug("EndCardAccess(%p)\n", handle));

    if (!ISMINE)
    	return FALSE;

    return TRUE;

    AROS_LIBFUNC_EXIT
}
