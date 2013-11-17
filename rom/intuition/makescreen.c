/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/graphics.h>
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH1(LONG, MakeScreen,

/*  SYNOPSIS */
        AROS_LHA(struct Screen *, screen, A0),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 63, Intuition)

/*  FUNCTION
        Create viewport of the screen.

    INPUTS
        Pointer to your custom screen.

    RESULT
        Zero for success, non-zero for failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        RemakeDisplay(), RethinkDisplay(), graphics.library/MakeVPort().

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    LONG  failure = TRUE;
    ULONG ilock = LockIBase(0);

    if (screen)
    {
        if ((screen->ViewPort.Modes ^ IntuitionBase->ViewLord.Modes) & LACE)
        {
            failure = RemakeDisplay();
        }
        else
        {
            failure = MakeVPort(&IntuitionBase->ViewLord, &screen->ViewPort);
        }
    }

    UnlockIBase(ilock);

    return failure;

    AROS_LIBFUNC_EXIT
} /* MakeScreen */
