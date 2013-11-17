/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH0(BOOL, WBenchToBack,

/*  SYNOPSIS */

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 56, Intuition)

/*  FUNCTION
        Bring the WorkBench behind all other screens.

    INPUTS

    RESULT
        TRUE if the Workbench screen is open, FALSE otherwise.

    NOTES
        This function does not influence the position of the screen,
        it just changes the depth-arrangement of the screens.

    EXAMPLE

    BUGS

    SEE ALSO
        ScreenToBack(), ScreenToFront(), WBenchToFront()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Screen *curscreen;

    curscreen = GetPrivIBase(IntuitionBase)->WorkBench;

    if ( curscreen )
    {
        ScreenToBack ( curscreen );
        return TRUE;
    }
    else
    {
        return FALSE;
    }


    AROS_LIBFUNC_EXIT
} /* WBenchToBack */
