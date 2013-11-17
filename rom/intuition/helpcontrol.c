/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH2(void, HelpControl,

/*  SYNOPSIS */
        AROS_LHA(struct Window *, window, A0),
        AROS_LHA(ULONG          , flags, D0),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 138, Intuition)

/*  FUNCTION
        Turn on or off Gadget-Help for your window. Gadget-Help will also be
        changed for all members of the same help-group to make
        multiple-windows apps behave well.

    INPUTS
        window - The window to affect. All windows of the same help-group
            will be affected as well.
        flags - HC_GADGETHELP or zero for turning help on or off.

    RESULT
        None.

    NOTES
        The WA_HelpGroup and WA_HelpGroupWindow tags are relevant to this
        function.

    EXAMPLE

    BUGS

    SEE ALSO
        OpenWindowTagList()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG ilock;
    ULONG clearmask = 0;
    ULONG setmask = 0;

    ASSERT_VALID_PTR(window);

    SANITY_CHECK(window)

    ilock = LockIBase(0);

    if (flags & HC_GADGETHELP)
    {
        setmask |= HELPF_GADGETHELP;
    }
    else
    {
        clearmask |= HELPF_GADGETHELP;
    }

#define CHANGEHELPFLAGS(x) ( IW(x)->helpflags = (IW(x)->helpflags | setmask) & ~clearmask )

    CHANGEHELPFLAGS(window);

    if (IW(window)->helpflags & HELPF_ISHELPGROUP)
    {
        struct Screen *scr = IntuitionBase->FirstScreen;

        for(; scr; scr = scr->NextScreen)
        {
            struct Window *win = scr->FirstWindow;

            for(; win; win = win->NextWindow)
            {
                if ( (IW(win)->helpflags & HELPF_ISHELPGROUP) &&
                        (IW(win)->helpgroup == IW(window)->helpgroup) )
                {
                    CHANGEHELPFLAGS(win);
                }
            }
        }
    }

    UnlockIBase(ilock);

    AROS_LIBFUNC_EXIT
} /* HelpControl */
