/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH0(BOOL, WBenchToFront,

/*  SYNOPSIS */

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 57, Intuition)

/*  FUNCTION
        Make the WorkBench screen the frontmost.

    INPUTS
        None.

    RESULT
        TRUE if the Workbench screen is open, FALSE else.

    NOTES
        This function does not influence the position of the screen,
        it just changes the depth-arrangement of the screens.

    EXAMPLE

    BUGS

    SEE ALSO
        ScreenToBack(), ScreenToFront(), WBenchToBack()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Screen *curscreen;

    curscreen = GetPrivIBase(IntuitionBase)->WorkBench;

    if ( curscreen )
    {
        ScreenToFront ( curscreen );
        return TRUE;
    }
    else
    {
        return FALSE;
    }

    AROS_LIBFUNC_EXIT
} /* WBenchToFront */
