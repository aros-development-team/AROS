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
struct HideWindowActionMsg
{
    struct IntuiActionMsg  msg;
    struct Window   	  *window;
};

static VOID int_hidewindow(struct HideWindowActionMsg *msg,
                           struct IntuitionBase *IntuitionBase);
#endif

#ifdef ChangeLayerVisibility
struct HideWindowActionMsg
{
    struct IntuiActionMsg   msg;
    struct Window   	  *window;
};

static VOID int_hidewindow(struct HideWindowActionMsg *msg,
                           struct IntuitionBase *IntuitionBase);
#endif

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH1(VOID, HideWindow,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 141, Intuition)

/*  FUNCTION
        Make a window invisible.
 
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
    struct HideWindowActionMsg msg;

    DEBUG_HIDEWINDOW(dprintf("HideWindow: Window 0x%lx\n", (ULONG) window));

    SANITY_CHECK(window)
    if (window->Flags & WFLG_BACKDROP) return;

    msg.window = window;
    DoASyncAction((APTR)int_hidewindow, &msg.msg, sizeof(msg), IntuitionBase);
#endif

#ifdef ChangeLayerVisibility
    struct HideWindowActionMsg msg;

    DEBUG_HIDEWINDOW(dprintf("HideWindow: Window 0x%lx\n", window));

    msg.window = window;
    DoASyncAction((APTR)int_hidewindow, &msg.msg, sizeof(msg), IntuitionBase);
#endif

    AROS_LIBFUNC_EXIT
} /* HideWindow */


#ifdef CGXSHOWHIDESUPPORT
static VOID int_hidewindow(struct HideWindowActionMsg *msg,
                           struct IntuitionBase *IntuitionBase)
{
    struct Window  *window = msg->window;
    struct Library *CGXSystemBase;

    if (!window) return;

    if (!ResourceExisting(window, RESOURCE_WINDOW, IntuitionBase)) return;

    if (window->Flags & WFLG_BACKDROP) return;

    if (((struct IntWindow *)(window))->specialflags & SPFLAG_NOICONIFY) return;

    if ((CGXSystemBase = OpenLibrary("cgxsystem.library", 0)))
    {
        CGXHideWindow(window);
        ((struct IntWindow *)(window))->specialflags |= SPFLAG_ICONIFIED;
        CloseLibrary(CGXSystemBase);
    }
};
#endif

#ifdef ChangeLayerVisibility
static VOID int_hidewindow(struct HideWindowActionMsg *msg,
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
            ChangeLayerVisibility(BLAYER(window), FALSE);
        }
        ChangeLayerVisibility(WLAYER(window), FALSE);

        for (req = window->FirstRequest; req; req = req->OlderRequest)
        {
            ChangeLayerVisibility(req->ReqLayer, FALSE);
        }

        UNLOCK_REFRESH(screen);

        CheckLayers(screen, IntuitionBase);
    }
}
#endif
