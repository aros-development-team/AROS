/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

        AROS_LH3(void, SizeWindow,

/*  SYNOPSIS */
        AROS_LHA(struct Window *, window, A0),
        AROS_LHA(LONG           , dx, D0),
        AROS_LHA(LONG           , dy, D1),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 48, Intuition)

/*  FUNCTION
        Modify the size of a window by the specified offsets.

    INPUTS
        window - The window to resize.
        dx - Add this to the width.
        dy - Add this to the height.

    RESULT
        None.

    NOTES
        The resize of the window may be delayed. If you depend on the
        information that is has changed size, wait for IDCMP_NEWSIZE.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    SANITY_CHECK(window)

    EXTENDWORD(dx);
    EXTENDWORD(dy);

    ChangeWindowBox(window, window->LeftEdge, window->TopEdge,
                    window->Width + dx, window->Height + dy);

    AROS_LIBFUNC_EXIT

} /* SizeWindow */
