/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/layers.h>
#include "showhide.h"
#include "intuition_intern.h"
#include "inputhandler_actions.h"

#ifdef CGXSHOWHIDESUPPORT
#undef ChangeLayerVisibility
struct ShowWindowActionMsg
{
    struct IntuiActionMsg    msg;
    struct Window   	    *window;
};

static VOID int_showwindow(struct ShowWindowActionMsg *msg,
                           struct IntuitionBase *IntuitionBase);
#endif

#ifdef ChangeLayerVisibility
struct ShowWindowActionMsg
{
    struct IntuiActionMsg    msg;
    struct Window   	    *window;
};

static VOID int_showwindow(struct ShowWindowActionMsg *msg,
                           struct IntuitionBase *IntuitionBase);
#endif

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH1(VOID, ShowWindow,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 140, Intuition)

/*  FUNCTION
        Make a window visible. This function does not bring the
        window back into the visible area of the screen but rather
        switches it into visible state.
 
 
    INPUTS
    window - The window to affect.
 
    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#ifdef CGXSHOWHIDESUPPORT
    struct ShowWindowActionMsg msg;

    DEBUG_SHOWWINDOW(dprintf("ShowWindow: Window 0x%lx\n", window));

    SANITY_CHECK(window)

    msg.window = window;
    DoASyncAction((APTR)int_showwindow, &msg.msg, sizeof(msg), IntuitionBase);
#endif

#ifdef ChangeLayerVisibility
    struct ShowWindowActionMsg msg;

    DEBUG_SHOWWINDOW(dprintf("ShowWindow: Window 0x%lx\n", window));

    msg.window = window;
    DoASyncAction((APTR)int_showwindow, &msg.msg, sizeof(msg), IntuitionBase);
#endif

    AROS_LIBFUNC_EXIT
} /* ShowWindow */

#ifdef CGXSHOWHIDESUPPORT
static VOID int_showwindow(struct ShowWindowActionMsg *msg,
                           struct IntuitionBase *IntuitionBase)
{
    struct Window  *window = msg->window;
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
};
#endif

#ifdef ChangeLayerVisibility
static VOID int_showwindow(struct ShowWindowActionMsg *msg,
                           struct IntuitionBase *IntuitionBase)
{
    struct Window *window = msg->window;
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
}
#endif
