/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/layers.h>
#include "intuition_intern.h"
#include "inputhandler_actions.h"

struct MoveWindowInFrontOfActionMsg
{
    struct IntuiActionMsg    msg;
    struct Window   	    *window;
    struct Window   	    *behindwindow;
};

static VOID int_movewindowinfrontof(struct MoveWindowInFrontOfActionMsg *msg,
                                    struct IntuitionBase *IntuitionBase);

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH2(void, MoveWindowInFrontOf,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),
         AROS_LHA(struct Window *, behindwindow, A1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 80, Intuition)

/*  FUNCTION
    Arrange the relative depth of a window.
 
    INPUTS
    window - the window to reposition
    behindwindow - the window the other one will be brought in front of
 
    RESULT
    None.
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    WindowToFront(), WindowToBack(), layers.library/MoveLayerInFrontOf()
 
    INTERNALS
    Uses layers.library/MoveLayerInFrontOf().
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct MoveWindowInFrontOfActionMsg msg;

    SANITY_CHECK(window)
    SANITY_CHECK(behindwindow)

    msg.window = window;
    msg.behindwindow = behindwindow;
    DoASyncAction((APTR)int_movewindowinfrontof, &msg.msg, sizeof(msg), IntuitionBase);

    AROS_LIBFUNC_EXIT

} /* MoveWindowInFrontOf */


static VOID int_movewindowinfrontof(struct MoveWindowInFrontOfActionMsg *msg,
                                    struct IntuitionBase *IntuitionBase)
{
    struct Window   	*window = msg->window;
    struct Window   	*behindwindow = msg->behindwindow;
    struct Screen   	*screen = window->WScreen;
    struct Requester 	*req;
    struct Layer    	*layer = WLAYER(window);
    struct Layer    	*lay;
    BOOL    	    	 movetoback = TRUE;
    
    if (!ResourceExisting(window, RESOURCE_WINDOW, IntuitionBase)) return;
    if (!ResourceExisting(behindwindow, RESOURCE_WINDOW, IntuitionBase)) return;

    LOCK_REFRESH(screen);

    for(lay = WLAYER(behindwindow); lay; lay = lay->back)
    {
        if (lay == layer)
        {
            movetoback = FALSE;
            break;
        }
    }

    /* FIXXXXXXXXXXXXXXXXXXXXXXXXXXXX FIXME FIXXXXXXXXXXXXXXXXXX */

    /* If GZZ window then also move outer window */

    MoveLayerInFrontOf(layer, WLAYER(behindwindow));

    if (BLAYER(window))
    {
        MoveLayerInFrontOf(BLAYER(window), BLAYER(behindwindow));
    }

    for (req = window->FirstRequest; req; req = req->OlderRequest)
    {
        if (req->ReqLayer)
        {
            MoveLayerInFrontOf(layer, req->ReqLayer);
        }
    }

    CheckLayers(screen, IntuitionBase);

    UNLOCK_REFRESH(screen);

    NotifyDepthArrangement(window, IntuitionBase);
}
