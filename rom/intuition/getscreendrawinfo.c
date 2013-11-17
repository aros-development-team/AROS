/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH1(struct DrawInfo *, GetScreenDrawInfo,

/*  SYNOPSIS */
        AROS_LHA(struct Screen *, screen, A0),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 115, Intuition)

/*  FUNCTION
        Returns a pointer to struct DrawInfo of the passed screen.
        This data is READ ONLY. The version of the struct DrawInfo
        is given in the dri_Version field.

    INPUTS
        screen - The screen you want to get the DrawInfo from.
            Must be valid and open.

    RESULT
        Returns pointer to struct DrawInfo defined in intuition/screens.h

    NOTES
        Call FreeScreenDrawInfo() after finishing using the pointer.
        This function does not prevent the screen from being closed.

    EXAMPLE

    BUGS

    SEE ALSO
        FreeScreenDrawInfo(), LockPubScreen(), intuition/screens.h

    INTERNALS
        Only returns the pointer.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ASSERT_VALID_PTR(screen);

    DEBUG_GETSCREENDRAWINFO(dprintf("GetScreenDrawInfo(screen 0x%lx)\n",screen));

    IntuitionBase = IntuitionBase;  /* shut up the compiler */

    SANITY_CHECKR(screen,FALSE)

    return (struct DrawInfo *)&(((struct IntScreen *)screen)->DInfo);

    AROS_LIBFUNC_EXIT
} /* GetScreenDrawInfo */
