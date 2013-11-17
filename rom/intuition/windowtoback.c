/*
    Copyright � 1995-2013, The AROS Development Team. All rights reserved.
    Copyright � 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$

    Move window behind all other windows.
*/

#include <proto/layers.h>
#include "intuition_intern.h"
#include "inputhandler_actions.h"

struct WindowToBackActionMsg
{
    struct IntuiActionMsg  msg;
    struct Window   	  *window;
};

static VOID int_windowtoback(struct WindowToBackActionMsg *msg,
                             struct IntuitionBase *IntuitionBase);

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

        AROS_LH1(void, WindowToBack,

/*  SYNOPSIS */
        AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 51, Intuition)

/*  FUNCTION
        Bring a window to the back (i.e. behind any other window).

    INPUTS
        window - Which window

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct WindowToBackActionMsg msg;

    DEBUG_WINDOWTOBACK(dprintf("WindowToBack: Window 0x%lx\n", window));

    if (!window)
        return;

    msg.window = window;
    DoASyncAction((APTR)int_windowtoback, &msg.msg, sizeof(msg), IntuitionBase);

    AROS_LIBFUNC_EXIT

} /* WindowToBack */


static VOID int_windowtoback(struct WindowToBackActionMsg *msg,
                             struct IntuitionBase *IntuitionBase)
{
    struct LayersBase   *LayersBase = GetPrivIBase(IntuitionBase)->LayersBase;
    struct Window   	*window = msg->window;
    struct Layer    	*layer = WLAYER(window);
    struct Screen   	*screen = window->WScreen;
    struct Requester 	*req;

    DEBUG_WINDOWTOBACK(dprintf("IntWindowToBack: Window 0x%lx\n", window));
    
    if (!ResourceExisting(window, RESOURCE_WINDOW, IntuitionBase)) return;
    
    if (!(layer->Flags & LAYERBACKDROP))
    {
        LOCK_REFRESH(screen);

        for (req = window->FirstRequest; req; req = req->OlderRequest)
        {
            if (req->ReqLayer)
            {
                BehindLayer(0, req->ReqLayer);
            }
        }

        BehindLayer(0, layer);

        /* GZZ window or regular window? */
        if (BLAYER(window))
        {
            /* move outer window behind! */
            /* attention: layer->back would not be valid as
               layer already moved!!
               */
            BehindLayer(0, BLAYER(window));
        }

        CheckLayers(screen, IntuitionBase);

        UNLOCK_REFRESH(screen);
    }

    NotifyDepthArrangement(window, IntuitionBase);
}
