/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    Move window in front of all other windows.
*/

#include <proto/layers.h>
#include "intuition_intern.h"
#include "inputhandler_actions.h"
#include "inputhandler_support.h"

struct WindowToFrontActionMsg
{
    struct IntuiActionMsg    msg;
    struct Window   	    *window;
};

static VOID int_windowtofront(struct WindowToFrontActionMsg *msg,
                              struct IntuitionBase *IntuitionBase);

/*****************************************************************************
 
    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

AROS_LH1(void, WindowToFront,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 52, Intuition)

/*  FUNCTION
    Bring a window to the front (ie. before any other window).
 
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
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct WindowToFrontActionMsg msg;

    DEBUG_WINDOWTOFRONT(dprintf("WindowToFront: Window 0x%lx\n", window));

    if (!window)
        return;

    msg.window = window;
    DoASyncAction((APTR)int_windowtofront, &msg.msg, sizeof(msg), IntuitionBase);

    AROS_LIBFUNC_EXIT

} /* WindowToFront */


static VOID int_windowtofront(struct WindowToFrontActionMsg *msg,
                              struct IntuitionBase *IntuitionBase)
{
    struct Window   	*window = msg->window;
    struct Layer    	*layer = WLAYER(window);
    struct Screen   	*screen = window->WScreen;
    struct Requester 	*req;

    DEBUG_WINDOWTOFRONT(dprintf("IntWindowToFront: Window 0x%lx\n", window));

    if (!ResourceExisting(window, RESOURCE_WINDOW, IntuitionBase)) return;

    if (!(layer->Flags & LAYERBACKDROP))
    {
        LOCK_REFRESH(screen);

        /* GZZ or regular window? */
        if (BLAYER(window))
        {
            /* bring outer window to front first!! */

            UpfrontLayer(NULL, BLAYER(window));
        }

        UpfrontLayer(NULL, layer);

        for (req = window->FirstRequest; req; req = req->OlderRequest)
        {
            if (req->ReqLayer)
            {
                MoveLayerInFrontOf(req->ReqLayer, layer);
            }
        }

        CheckLayers(screen, IntuitionBase);

        UNLOCK_REFRESH(screen);

    }

    ((struct IntWindow *)(window))->specialflags &= ~SPFLAG_ICONIFIED;
    /* window is not iconified anymore */

    NotifyDepthArrangement(window, IntuitionBase);
}
