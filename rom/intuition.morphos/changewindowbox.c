/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    Change position and size of a window.
*/

#include "intuition_intern.h"
#include "inputhandler_actions.h"

struct ChangeWindowBoxActionMsg
{
    struct IntuiActionMsg msg;
    struct Window *window;
    LONG left;
    LONG top;
    LONG width;
    LONG height;
};

static VOID int_changewindowbox(struct ChangeWindowBoxActionMsg *msg,
                                struct IntuitionBase *IntuitionBase);

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH5(void, ChangeWindowBox,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),
         AROS_LHA(LONG           , left, D0),
         AROS_LHA(LONG           , top, D1),
         AROS_LHA(LONG           , width, D2),
         AROS_LHA(LONG           , height, D3),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 81, Intuition)

/*  FUNCTION
    Set the new position and size of a window in one call.
 
    INPUTS
    window - Change this window
    left, top - New position
    width, height - New size
 
    RESULT
 
    NOTES
    This call is deferred. Wait() for IDCMP_CHANGEWINDOW if your
    program depends on the new size.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct ChangeWindowBoxActionMsg msg;

    DEBUG_CHANGEWINDOWBOX(dprintf("ChangeWindowBox: Window 0x%lx Left %d Top %d Width %d Height %d\n",
                                  window, left, top, width, height));

    if (!window)
        return;
    
    if (!ResourceExisting(window, RESOURCE_WINDOW, IntuitionBase)) return;
    
    EXTENDWORD(left);
    EXTENDWORD(top);
    EXTENDWORD(width);
    EXTENDWORD(height);

    msg.window = window;
    msg.left   = left;
    msg.top    = top;

    msg.width  = width;
    if (window->Flags & WFLG_SIZEGADGET)
    {
        if (width < window->MinWidth) width = window->MinWidth;
        if (width > window->MaxWidth) width = window->MaxWidth;
    }
    if (width > window->WScreen->Width) msg.width = window->WScreen->Width;

    msg.height = height;
    if (window->Flags & WFLG_SIZEGADGET)
    {
        if (height < window->MinHeight) height = window->MinHeight;
        if (height > window->MaxHeight) height = window->MaxHeight;
    }
    if (height > window->WScreen->Height) msg.height = window->WScreen->Height;

    DoASyncAction((APTR)int_changewindowbox, &msg.msg, sizeof(msg), IntuitionBase);
//    DoSyncAction((APTR)int_changewindowbox, &msg.msg, IntuitionBase);

    AROS_LIBFUNC_EXIT

} /* ChangeWindowBox */


static VOID int_changewindowbox(struct ChangeWindowBoxActionMsg *msg,
                                struct IntuitionBase *IntuitionBase)
{
    struct Window *window = msg->window;

    DoMoveSizeWindow(window, msg->left, msg->top, msg->width, msg->height, TRUE, IntuitionBase);
}
