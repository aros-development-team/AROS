/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/layers.h>
#include "showhide.h"
#include "intuition_intern.h"
#include "inputhandler_actions.h"

struct ShowWindowActionMsg
{
    struct IntuiActionMsg    msg;
    struct Window   	    *window;
};

static VOID int_showwindow(struct ShowWindowActionMsg *msg,
                           struct IntuitionBase *IntuitionBase);

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

    AROS_LH2(BOOL, ShowWindow,

/*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),
         AROS_LHA(struct Window *, other, A1),

/*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 140, Intuition)

/*  FUNCTION
        Make a window visible. This function does not bring the
        window back into the visible area of the screen but rather
        switches it into visible state.
 
    INPUTS
	window - The window to affect.
 
    RESULT
	Success indicator. On AROS it's always TRUE.
 
    NOTES
	This function is soure-compatible with AmigaOS v4.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
	HideWindow()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ShowWindowActionMsg msg;

    DEBUG_SHOWWINDOW(dprintf("ShowWindow: Window 0x%p\n", window));
    SANITY_CHECKR(window, FALSE)

    /*
     * TODO: in AmigaOS v4 we have additional 'other' parameter,
     * and the window is moved in front of that window. Implement this.
     */

    msg.window = window;
    DoASyncAction((APTR)int_showwindow, &msg.msg, sizeof(msg), IntuitionBase);

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* ShowWindow */

static VOID int_showwindow(struct ShowWindowActionMsg *msg,
                           struct IntuitionBase *IntuitionBase)
{
    struct LayersBase *LayersBase = GetPrivIBase(IntuitionBase)->LayersBase;
    struct Window  *window = msg->window;
#ifdef CGXSHOWHIDESUPPORT
    struct Library *CGXSystemBase;
    
    if (!ResourceExisting(window, RESOURCE_WINDOW, IntuitionBase)) return;
    
    SANITY_CHECK(window)
    if ((CGXSystemBase = OpenLibrary("cgxsystem.library", 0)))
    {
        CGXShowWindow(window);
        ((struct IntWindow *)(window))->specialflags &= ~SPFLAG_ICONIFIED;
        CloseLibrary(CGXSystemBase);
        ActivateWindow(window);
    }
#else
    struct Screen *screen;
    
    if (!ResourceExisting(window, RESOURCE_WINDOW, IntuitionBase)) return;
    screen = window->WScreen;

    if (!IsWindowVisible(window))
    {
        struct Requester *req;

        LOCK_REFRESH(screen);

        if (BLAYER(window))
        {
            ChangeLayerVisibility(BLAYER(window), TRUE);
        }
        ChangeLayerVisibility(WLAYER(window), TRUE);

        for (req = window->FirstRequest; req; req = req->OlderRequest)
        {
            ChangeLayerVisibility(req->ReqLayer, TRUE);
        }

        CheckLayers(screen, IntuitionBase);

        UNLOCK_REFRESH(screen);
    }


#endif

};
