/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/layers.h>
#include "intuition_intern.h"
#include "inputhandler_actions.h"

struct ShowTitleActionMsg
{
    struct IntuiActionMsg    msg;
    struct Screen   	    *screen;
    BOOL    	    	     showit;
};

static VOID int_showtitle(struct ShowTitleActionMsg *msg,
                          struct IntuitionBase *IntuitionBase);

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH2(void, ShowTitle,

         /*  SYNOPSIS */
         AROS_LHA(struct Screen *, screen, A0),
         AROS_LHA(BOOL           , ShowIt, D0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 47, Intuition)

/*  FUNCTION
    Modify SHOWTITLE flag of the screen and refresh the screen and
    its windows.
    If ShowIt is TRUE the screen's title bar will be shown in front of
    WFLG_BACKDROP windows. A value of FALSE will bring the title bar
    behind all windows.
 
    INPUTS
 
    RESULT
    None.
 
    NOTES
    The default of the SHOWTITLE flag for new screens is TRUE.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    if (screen && screen->BarLayer)
    {
        struct ShowTitleActionMsg msg;

    #ifdef SKINS
        if (GetPrivScreen(screen)->SpecialFlags & (SF_InvisibleBar|SF_AppearingBar)) return;
    #endif

        msg.screen = screen;
        msg.showit = ShowIt;
        DoASyncAction((APTR)int_showtitle, &msg.msg, sizeof(msg), IntuitionBase);
    }

    AROS_LIBFUNC_EXIT

} /* ShowTitle */


static VOID int_showtitle(struct ShowTitleActionMsg *msg,
                          struct IntuitionBase *IntuitionBase)
{
    struct Screen *screen = msg->screen;

    if (msg->showit)
    {
        if (screen->Flags & SHOWTITLE)
        {
            LOCK_REFRESH(screen);

            BehindLayer(0, screen->BarLayer);

            AROS_ATOMIC_AND(screen->Flags, ~SHOWTITLE);

            CheckLayers(screen, IntuitionBase);

            UNLOCK_REFRESH(screen);
        }
    }
    else
    {
        if (!(screen->Flags & SHOWTITLE))
        {
            LOCK_REFRESH(screen);

            UpfrontLayer(0, screen->BarLayer);

            AROS_ATOMIC_OR(screen->Flags, SHOWTITLE);

            CheckLayers(screen, IntuitionBase);

            UNLOCK_REFRESH(screen);
        }
    }
}
