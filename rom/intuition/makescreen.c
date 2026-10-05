/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
    Copyright (C) 2001-2003, The MorphOS Development Team. All Rights Reserved.
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
        struct ViewPortExtra *vpe = (struct ViewPortExtra *)GfxLookUp(&screen->ViewPort);

        /* Classic applications update Screen's dimensions directly before
         * rebuilding its display. The backing bitmap can remain larger than
         * the visible screen; displaying its full height exposes old pixels. */
        screen->ViewPort.DxOffset = screen->LeftEdge;
        screen->ViewPort.DyOffset = screen->TopEdge;
        screen->ViewPort.DWidth = screen->Width;
        screen->ViewPort.DHeight = screen->Height;
        if (vpe)
        {
            WORD width = vpe->DisplayClip.MaxX - vpe->DisplayClip.MinX + 1;
            WORD height = vpe->DisplayClip.MaxY - vpe->DisplayClip.MinY + 1;

            if (screen->ViewPort.DWidth > width)
                screen->ViewPort.DWidth = width;
            if (screen->ViewPort.DHeight > height)
                screen->ViewPort.DHeight = height;
        }

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
