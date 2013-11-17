/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/layers.h>
#include "showhide.h"
#include "intuition_intern.h"
#include "inputhandler_actions.h"

struct HideWindowActionMsg
{
    struct IntuiActionMsg  msg;
    struct Window   	  *window;
};

static VOID int_hidewindow(struct HideWindowActionMsg *msg,
                           struct IntuitionBase *IntuitionBase);

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH1(BOOL, HideWindow,

/*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 141, Intuition)

/*  FUNCTION
        Make a window invisible.

    INPUTS
	window - The window to affect.

    RESULT
	Success indicator. On AROS this is always TRUE.

    NOTES
	This function is source-compatible with AmigaOS v4.
        This function is also present in MorphOS v50, however
        considered private.

    EXAMPLE

    BUGS

    SEE ALSO
	ShowWindow()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct HideWindowActionMsg msg;

    DEBUG_HIDEWINDOW(dprintf("HideWindow: Window 0x%lx\n", (ULONG) window));
    SANITY_CHECKR(window, FALSE)

#ifdef CGXSHOWHIDESUPPORT
    if (window->Flags & WFLG_BACKDROP) return;
#endif

    msg.window = window;
    DoASyncAction((APTR)int_hidewindow, &msg.msg, sizeof(msg), IntuitionBase);

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* HideWindow */


static VOID int_hidewindow(struct HideWindowActionMsg *msg,
                           struct IntuitionBase *IntuitionBase)
{
    struct LayersBase *LayersBase = GetPrivIBase(IntuitionBase)->LayersBase;
    struct Window  *window = msg->window;
#ifdef CGXSHOWHIDESUPPORT
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
#else
    struct Screen *screen;

    if (!ResourceExisting(window, RESOURCE_WINDOW, IntuitionBase)) return;

    screen = window->WScreen;
    
    if (IsWindowVisible(window))
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
#endif
};
