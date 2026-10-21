/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.

    Desc: EndCardAccess() function.
*/

#include "card_intern.h"

/*****************************************************************************
    NAME
        BOOL EndCardAccess(

    SYNOPSIS
        struct CardHandle *handle)

    FUNCTION
        Test whether handle owns a card currently marked present by the
        m68k-Amiga resource.

    INPUTS
        handle - Non-NULL CardHandle used for the ownership request.

    RESULT
        TRUE if handle equals the current owner and the card is not marked
        removed; otherwise FALSE.

    NOTES
        This call does not acquire or release a lock, change ownership, map
        memory or reserve access. TRUE is a snapshot, not a guarantee that
        the card remains present. Continue to handle removal notifications.

*****************************************************************************/
AROS_LH1(ULONG, EndCardAccess,
        AROS_LHA(struct CardHandle*, handle, A1),
        struct CardResource*, CardResource, 5, Card)
{
    AROS_LIBFUNC_INIT

    CARDDEBUG(bug("EndCardAccess(%p)\n", handle));

    if (!ISMINE)
        return FALSE;

    return TRUE;

    AROS_LIBFUNC_EXIT
}
