/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"
#include "inputhandler_actions.h"

struct ZipWindowActionMsg
{
    struct IntuiActionMsg  msg;
    struct Window   	  *window;
};

static VOID int_zipwindow(struct ZipWindowActionMsg *msg,
                          struct IntuitionBase *IntuitionBase);

/*****************************************************************************
 
    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

AROS_LH1(void, ZipWindow,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *, window, A0),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 84, Intuition)

/*  FUNCTION
    "Zip" (move and resize) a window to the coordinates and dimensions
    the window had at the last call of ZipWindow(), or invoked via the
    zoom-gadget.
 
    INPUTS
    window - Which window
 
    RESULT
    None.
 
    NOTES
    This call is deferred. Wait() for IDCMP_CHANGEWINDOW if your
    program depends on the new size.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    ChangeWindowBox(), MoveWindow(), SizeWindow()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct ZipWindowActionMsg msg;

    DEBUG_ZIPWINDOW(dprintf("ZipWindow(Window 0x%lx)\n",window));

    SANITY_CHECK(window)

    msg.window = window;

    DoASyncAction((APTR)int_zipwindow, &msg.msg, sizeof(msg), IntuitionBase);
//    DoSyncAction((APTR)int_zipwindow, &msg.msg, IntuitionBase);

    AROS_LIBFUNC_EXIT

} /* ZipWindow */


static VOID int_zipwindow(struct ZipWindowActionMsg *msg,
                          struct IntuitionBase *IntuitionBase)
{
    struct Window   	*window = msg->window;
    struct IntWindow 	*w = (struct IntWindow *)window;
    LONG    	    	 NewLeftEdge, NewTopEdge, NewWidth, NewHeight;

    if (!ResourceExisting(window, RESOURCE_WINDOW, IntuitionBase)) return;
    
    NewLeftEdge = window->LeftEdge;
    if (w->ZipLeftEdge != ~0)
    {
        NewLeftEdge    = w->ZipLeftEdge;
    #ifdef __MORPHOS__
        w->ZipLeftEdge = w->window.LeftEdge;
    #else
        w->ZipLeftEdge = w->window.RelLeftEdge;
    #endif
    }

    NewTopEdge = window->TopEdge;
    if (w->ZipTopEdge != ~0)
    {
        NewTopEdge    = w->ZipTopEdge;
    #ifdef __MORPHOS__
        w->ZipTopEdge = w->window.TopEdge;
    #else
        w->ZipTopEdge = w->window.RelTopEdge;
#   endif
    }

    NewWidth = window->Width;
    if (w->ZipWidth != ~0)
    {
        NewWidth    = w->ZipWidth;
	if (window->Flags & WFLG_SIZEGADGET)
	{
            if (NewWidth < window->MinWidth) NewWidth = window->MinWidth;
            if (NewWidth > window->MaxWidth) NewWidth = window->MaxWidth;
	}
        w->ZipWidth = w->window.Width;
    }

    NewHeight = window->Height;
    if (w->ZipHeight != ~0)
    {
        NewHeight    = w->ZipHeight;
	if (window->Flags & WFLG_SIZEGADGET)
	{
            if (NewHeight < window->MinHeight) NewHeight = window->MinHeight;
            if (NewHeight > window->MaxHeight) NewHeight = window->MaxHeight;
	}
        w->ZipHeight = w->window.Height;
    }

    DoMoveSizeWindow(window, NewLeftEdge, NewTopEdge, NewWidth, NewHeight, TRUE, IntuitionBase);
}
