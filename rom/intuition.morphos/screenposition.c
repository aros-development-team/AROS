/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

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
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/ScreenPosition()
    //    aros_print_not_implemented ("ScreenPosition");

    /* shut up the compiler */
    IntuitionBase = IntuitionBase;
    screen = screen;
    flags = flags;
    x1 = x1;
    x2 = x2;
    y1 = y1;
    y2 = y2;

    AROS_LIBFUNC_EXIT
} /* ScreenPosition */
