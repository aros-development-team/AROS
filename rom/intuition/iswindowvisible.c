/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    Copyright � 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>
#include <proto/layers.h>

        AROS_LH1(LONG, IsWindowVisible,

/*  SYNOPSIS */
        AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 139, Intuition)

/*  FUNCTION
        Check whether a window is visible or not. This does not
        check whether the window is within the visible area of
        the screen but rather whether it is in visible state.

    INPUTS
        window - The window to affect.

    RESULT
        TRUE if window is currently visible, FALSE otherwise.

    NOTES
        This function is also present in MorphOS v50, however
        considered private.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct LayersBase *LayersBase = GetPrivIBase(IntuitionBase)->LayersBase;

    SANITY_CHECKR(window,FALSE)

    return IsLayerVisible(WLAYER(window));

    AROS_LIBFUNC_EXIT
} /* IsWindowVisible */
