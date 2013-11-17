/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    Copyright � 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/graphics.h>
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH0(LONG, RemakeDisplay,

/*  SYNOPSIS */

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 64, Intuition)

/*  FUNCTION
        Remake the entire Intuition display.

    INPUTS
        None.

    RESULT
        Zero for success, non-zero for failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        RethinkDisplay(), MakeScreen(), graphics.library/MakeVPort(),
        graphics.library/MrgCop(), graphics.library/LoadView()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    struct Screen   *screen;
    ULONG   	     ilock = LockIBase(0);
    LONG    	     failure = 0;

    for (screen = IntuitionBase->FirstScreen; screen; screen = screen->NextScreen)
    {
        LONG error = MakeVPort(&IntuitionBase->ViewLord, &screen->ViewPort);

        if (error)
            failure = error;
    }

    if (!failure)
        failure = RethinkDisplay();

    UnlockIBase(ilock);

    return failure;

    AROS_LIBFUNC_EXIT
} /* RemakeDisplay */
