/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$

    Move a screen in front of all other screens.
*/

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH1(void, ScreenToFront,

/*  SYNOPSIS */
        AROS_LHA(struct Screen *, screen, A0),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 42, Intuition)

/*  FUNCTION
        Move a screen in front of all other screens. If the screen is in a
        group, the screen will be moved in front of all other screens in the
        group only. If the screen is the parent of a group, the whole
        group will be moved in the front.

    INPUTS
        screen - Move this screen.

    RESULT
        You will see the screen move in front of all other screens.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        ScreenToBack(), ScreenDepth()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ScreenDepth ( screen, SDEPTH_TOFRONT, NULL );

    AROS_LIBFUNC_EXIT
} /* ScreenToFront */
