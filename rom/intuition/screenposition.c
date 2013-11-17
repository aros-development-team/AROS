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

    if ((flags & SPOS_FORCEDRAG) || (GetPrivScreen(screen)->SpecialFlags & SF_Draggable)) {
    
	/* First we update the viewport, then attempt to scroll. The bitmap may refuse to scroll
	   too far, in this case offsets in the ViewPort will be adjusted to reflect the real situation.
	   TODO: check if additional bounding has to be implemented. Graphics driver could for example let
	   to scroll the bitmap completely out of the display. */
        if (flags & SPOS_ABSOLUTE) {
	    D(bug("[ScreenPosition] Absolute position: (%d, %d)\n", x1, y1));
	    screen->ViewPort.DxOffset = x1;
	    screen->ViewPort.DyOffset = y1;
	} else {
	    D(bug("[ScreenPosition] Relative position: (%d, %d)\n", x1, y1));
	    screen->ViewPort.DxOffset = screen->LeftEdge + x1;
	    screen->ViewPort.DyOffset = screen->TopEdge  + y1;
	}
	D(bug("[ScreenPosition] Scroll to: (%d, %d)\n",screen->ViewPort.DxOffset, screen->ViewPort.DyOffset));
	ScrollVPort(&screen->ViewPort);

	/* Bring back the actual resulting values to our screen structure */
	D(bug("[ScreenPosition] Scroll result: (%d, %d)\n",screen->ViewPort.DxOffset, screen->ViewPort.DyOffset));
	screen->LeftEdge = screen->ViewPort.DxOffset;
	screen->TopEdge = screen->ViewPort.DyOffset;
    }

    AROS_LIBFUNC_EXIT
} /* ScreenPosition */
