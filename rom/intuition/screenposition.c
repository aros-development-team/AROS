/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
    Copyright (C) 2001-2003, The MorphOS Development Team. All Rights Reserved.
*/

#include <graphics/view.h>
#include <proto/graphics.h>

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH6(void, ScreenPosition,

/*  SYNOPSIS */
        AROS_LHA(struct Screen *, screen, A0),
        AROS_LHA(ULONG          , flags, D0),
        AROS_LHA(LONG           , x1, D1),
        AROS_LHA(LONG           , y1, D2),
        AROS_LHA(LONG           , x2, D3),
        AROS_LHA(LONG           , y2, D4),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 132, Intuition)

/*  FUNCTION
        Move a screen to the specified position or by the specified
        increment. Resolution is always the screen resolution.
        If this move would be out of bounds, the move is clipped at
        these boundaries. The real new position can be obtained from
        LeftEdge and TopEdge of the screen's structure.

    INPUTS
        screen - Move this screen
        flags - One of SPOS_RELATIVE, SPOS_ABSOLUTE or SPOS_MAKEVISIBLE
            Use SPOS_FORCEDRAG to override non-movable screens ie. screens
            opened with {SA_Draggable,FLASE} attribute.

            SPOS_RELATIVE (or NULL) moves the screen by a delta of x1,y1.

            SPOS_ABSOLUTE moves the screen to the specified position x1,y1.

            SPOS_MAKEVISIBLE moves an oversized scrolling screen to make
            the rectangle (x1,y1),(x2,y2) visible
        x1,y1 - Absolute (SPOS_ABSOLUTE) or relative (SPOS_RELATIVE) coordinate
            to move screen, or upper-left corner of rectangle
            (SPOS_MAKEVISIBLE)
        x2,y2 - Ignored with SPOS_ABSOLUTE and SPOS_RELATIVE.
            Lower-right corner of rectangle with SPOS_MAKEVISIBLE.

    RESULT
        None.

    NOTES
        SPOS_FORCEDRAG should only be used by the owner of the screen.

    EXAMPLE

    BUGS

    SEE ALSO
        MoveScreen(), RethinkDisplay()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    ULONG mode = flags & (SPOS_ABSOLUTE | SPOS_MAKEVISIBLE);
    ULONG ilock;

    SANITY_CHECK(screen)

    ilock = LockIBase(0);

    if (((flags & SPOS_FORCEDRAG) || (GetPrivScreen(screen)->SpecialFlags & SF_Draggable)) &&
        mode != (SPOS_ABSOLUTE | SPOS_MAKEVISIBLE))
    {
        /* First we update the viewport, then attempt to scroll. The bitmap may refuse to scroll
           too far, in this case offsets in the ViewPort will be adjusted to reflect the real situation. */
        if (mode == SPOS_ABSOLUTE)
        {
            D(bug("[ScreenPosition] Absolute position: (%d, %d)\n", x1, y1));
            screen->ViewPort.DxOffset = x1;
            screen->ViewPort.DyOffset = y1;
        }
        else if (mode == SPOS_MAKEVISIBLE)
        {
            struct ViewPortExtra *vpe = (struct ViewPortExtra *)GfxLookUp(&screen->ViewPort);
            LONG dwidth = (screen->ViewPort.DWidth > 0) ?
                          (LONG)screen->ViewPort.DWidth : (LONG)screen->Width;
            LONG dheight = (screen->ViewPort.DHeight > 0) ?
                           (LONG)screen->ViewPort.DHeight : (LONG)screen->Height;
            QUAD rx1 = x1;
            QUAD ry1 = y1;
            QUAD rx2 = (x2 < x1) ? x1 : x2;
            QUAD ry2 = (y2 < y1) ? y1 : y2;
            QUAD new_x = screen->LeftEdge;
            QUAD new_y = screen->TopEdge;

            if (!vpe && screen->ViewPort.ColorMap)
                vpe = screen->ViewPort.ColorMap->cm_vpe;
            if (vpe)
            {
                /* OpenScreen(), MakeScreen(), and MonitorClass derive the visible
                   display span from DisplayClip width/height (Max - Min + 1) while
                   keeping Screen LeftEdge/TopEdge and ViewPort DxOffset/DyOffset
                   0-based relative to the display clip aperture even when
                   DisplayClip.MinX or MinY is nonzero (such as overscan clips). */
                LONG clip_w = (LONG)vpe->DisplayClip.MaxX - (LONG)vpe->DisplayClip.MinX + 1;
                LONG clip_h = (LONG)vpe->DisplayClip.MaxY - (LONG)vpe->DisplayClip.MinY + 1;

                if (clip_w > 0 && (vpe->DisplayClip.MinX != 0 || vpe->DisplayClip.MaxX != 0))
                    dwidth = clip_w;
                if (clip_h > 0 && (vpe->DisplayClip.MinY != 0 || vpe->DisplayClip.MaxY != 0))
                    dheight = clip_h;
            }

            D(bug("[ScreenPosition] Make visible: (%d, %d)-(%d, %d)\n", x1, y1, x2, y2));
            if (dwidth > 0)
            {
                if (new_x + rx2 >= dwidth)
                    new_x = (QUAD)dwidth - 1 - rx2;
                if (new_x + rx1 < 0)
                    new_x = -rx1;
            }
            if (dheight > 0)
            {
                if (new_y + ry2 >= dheight)
                    new_y = (QUAD)dheight - 1 - ry2;
                if (new_y + ry1 < 0)
                    new_y = -ry1;
            }

            if (dwidth > 0 && screen->Width > 0)
            {
                QUAD min_x = (screen->Width > dwidth) ?
                             ((QUAD)dwidth - (QUAD)screen->Width) : 0;
                QUAD max_x = (screen->LeftEdge > 0) ? (QUAD)screen->LeftEdge : 0;

                if ((QUAD)screen->LeftEdge < min_x)
                    min_x = (QUAD)screen->LeftEdge;
                if (new_x < min_x)
                    new_x = min_x;
                if (new_x > max_x)
                    new_x = max_x;
            }
            if (dheight > 0 && screen->Height > 0)
            {
                QUAD min_y = (screen->Height > dheight) ?
                             ((QUAD)dheight - (QUAD)screen->Height) : 0;
                QUAD max_y = (screen->TopEdge > 0) ? (QUAD)screen->TopEdge : 0;

                if ((QUAD)screen->TopEdge < min_y)
                    min_y = (QUAD)screen->TopEdge;
                if (new_y < min_y)
                    new_y = min_y;
                if (new_y > max_y)
                    new_y = max_y;
            }
            if (new_x < -32768)
                new_x = -32768;
            if (new_x > 32767)
                new_x = 32767;
            if (new_y < -32768)
                new_y = -32768;
            if (new_y > 32767)
                new_y = 32767;

            screen->ViewPort.DxOffset = (WORD)new_x;
            screen->ViewPort.DyOffset = (WORD)new_y;
        }
        else
        {
            D(bug("[ScreenPosition] Relative position: (%d, %d)\n", x1, y1));
            screen->ViewPort.DxOffset = (WORD)((QUAD)screen->LeftEdge + (QUAD)x1);
            screen->ViewPort.DyOffset = (WORD)((QUAD)screen->TopEdge  + (QUAD)y1);
        }

        D(bug("[ScreenPosition] Scroll to: (%d, %d)\n",screen->ViewPort.DxOffset, screen->ViewPort.DyOffset));
        ScrollVPort(&screen->ViewPort);

        /* Bring back the actual resulting values to our screen structure */
        D(bug("[ScreenPosition] Scroll result: (%d, %d)\n",screen->ViewPort.DxOffset, screen->ViewPort.DyOffset));
        screen->LeftEdge = screen->ViewPort.DxOffset;
        screen->TopEdge = screen->ViewPort.DyOffset;
    }

    UnlockIBase(ilock);

    AROS_LIBFUNC_EXIT
} /* ScreenPosition */
