/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.

    Desc: CardChangeCount() function.
*/

#include <proto/exec.h>

#include "card_intern.h"

/*****************************************************************************
    NAME
        ULONG CardChangeCount(

    SYNOPSIS
        )

    FUNCTION
        Read the m68k-Amiga resource's processed-change counter.

    RESULT
        The current ULONG counter value.

    NOTES
        The resource task increments the counter after waking for a change,
        before debouncing and processing it. Multiple signals may coalesce.
        CardForceChange() does not increment it directly, and counter
        equality does not prove that the same card is still present. Use the
        value as a change indicator; it can wrap.

*****************************************************************************/
AROS_LH0(ULONG, CardChangeCount,
        struct CardResource*, CardResource, 16, Card)
{
    AROS_LIBFUNC_INIT

    return CardResource->changecount;

    AROS_LIBFUNC_EXIT
}
