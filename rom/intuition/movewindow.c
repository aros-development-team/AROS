/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    Move a window around on the screen.
*/

#include "intuition_intern.h"
#include "inputhandler_actions.h"

struct MoveWindowActionMsg
{
    struct IntuiActionMsg    msg;
    struct Window   	    *window;
    LONG    	    	     dx;
    LONG    	    	     dy;
};

static VOID int_movewindow(struct MoveWindowActionMsg *msg,
                           struct IntuitionBase *IntuitionBase);


/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH3(void, MoveWindow,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),
         AROS_LHA(LONG           , dx, D0),
         AROS_LHA(LONG           , dy, D1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 28, Intuition)

/*  FUNCTION
    Change the position of a window on the screen.
 
    INPUTS
    window - Move this window
    dx, dy - Move it that many pixels along the axis (right, down)
 
    RESULT
    The window will move when the next input event will be received.
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    SizeWindow()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    EXTENDWORD(dx);
    EXTENDWORD(dy);

    if (window && (dx || dy))
    {
        struct MoveWindowActionMsg msg;

        msg.window = window;
        msg.dx     = dx;
        msg.dy     = dy;

        DoASyncAction((APTR)int_movewindow, &msg.msg, sizeof(msg), IntuitionBase);
        //DoSyncAction((APTR)int_movewindow, &msg.msg, IntuitionBase);
    }

    AROS_LIBFUNC_EXIT
} /* MoveWindow */

static VOID int_movewindow(struct MoveWindowActionMsg *msg,
                           struct IntuitionBase *IntuitionBase)
{
    struct Window *window = msg->window;

    if (!ResourceExisting(window, RESOURCE_WINDOW, IntuitionBase)) return;

    DoMoveSizeWindow(window,
                     window->LeftEdge + msg->dx, window->TopEdge + msg->dy,
                     window->Width, window->Height, FALSE, IntuitionBase);
}
