/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.

    Desc: IfAmigaXIP() function.
*/

#include "card_intern.h"

/*****************************************************************************
    NAME
        struct Resident * IfAmigaXIP(

    SYNOPSIS
        struct CardHandle *handle)

    FUNCTION
        Inspect the m68k-Amiga card's execute-in-place tuple without
        returning a resident.

    INPUTS
        handle - Current card owner.

    RESULT
        Always NULL.

    NOTES
        When the ownership/presence check succeeds, the function calls
        CopyTuple() for CISTPL_AMIGAXIP and decodes its location field. No
        resident is validated or executed. CopyTuple() traversal and
        blocking limitations apply.

    BUGS
        Execute-in-place support is incomplete. NULL does not distinguish an
        absent tuple, a read failure or a tuple that this implementation
        cannot use.

*****************************************************************************/
AROS_LH1(struct Resident*, IfAmigaXIP,
        AROS_LHA(struct CardHandle*, handle, A2),
        struct CardResource*, CardResource, 14, Card)
{
    AROS_LIBFUNC_INIT

    struct TP_AmigaXIP xip;
    ULONG addr;

    if (!ISMINE)
        return NULL;
        
    if (!CopyTuple(handle, (UBYTE*)&xip, CISTPL_AMIGAXIP, sizeof(xip) - 2))
        return FALSE;
    if (xip.TPL_LINK <= 6)
        return FALSE;
    addr = (xip.TP_XIPLOC[3] << 24) | (xip.TP_XIPLOC[2] << 16) | (xip.TP_XIPLOC[1] << 8) | (xip.TP_XIPLOC[0] << 0);

    CARDDEBUG(bug("TP_AmigaXIP found, addr %p\n", addr));

    /* TODO */
    return NULL;

    AROS_LIBFUNC_EXIT
}
