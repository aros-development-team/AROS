/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH3(void, MoveScreen,

         /*  SYNOPSIS */
         AROS_LHA(struct Screen *, screen, A0),
         AROS_LHA(LONG           , dx, D0),
         AROS_LHA(LONG           , dy, D1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 27, Intuition)

/*  FUNCTION
    Move a screen by the specified amount in X/Y direction. The
    resolution is always the screen resolution.
 
    INPUTS
    screen - Move this screen
    dx - Move it by this amount along the X axis (> 0 to the right,
        < 0 to the left).
    dy - Move it by this amount along the Y axis (> 0 down, < 0 up)
 
    RESULT
    None.
 
    NOTES
    Depending on other restrictions, the screen may not move as far
    as specified. It will move as far as possible and you can check
    LeftEdge and TopEdge of the screen to see how far it got.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    RethinkDisplay()
 
    INTERNALS
 
    HISTORY
    27-11-96    digulla automatically created from
                intuition_lib.fd and clib/intuition_protos.h
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    ScreenPosition(screen, SPOS_RELATIVE, dx, dy, 0, 0);

    AROS_LIBFUNC_EXIT
} /* MoveScreen */
