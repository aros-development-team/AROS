/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/layers.h>
#include "intuition_intern.h"
#include "inputhandler_actions.h"

struct RequestActionMsg
{
    struct IntuiActionMsg    msg;
    struct Requester 	    *requester;
    struct Window   	    *window;
    BOOL    	    	     success;
};

static VOID int_request(struct RequestActionMsg *msg,
                        struct IntuitionBase *IntuitionBase);

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

AROS_LH2(BOOL, Request,

         /*  SYNOPSIS */
         AROS_LHA(struct Requester *, requester, A0),
         AROS_LHA(struct Window *   , window, A1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 40, Intuition)

/*  FUNCTION
    Add a requester to specified window and display it.

    INPUTS
    requester - The requester to be displayed
    window - The window to which the requester belongs

    RESULT
    TRUE if requester was opened successfully, FALSE else.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
    EndRequest(), InitRequester()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct RequestActionMsg msg;

    DEBUG_REQUEST(dprintf("Request: requester 0x%lx window 0x%lx\n", requester, window));

    SANITY_CHECKR(window,FALSE)
    SANITY_CHECKR(requester,FALSE)

    msg.window    = window;
    msg.requester = requester;

    DoSyncAction((APTR)int_request, &msg.msg, IntuitionBase);

    DEBUG_REQUEST(dprintf("Request: requester (layer 0x%lx) setup %d\n",requester->ReqLayer,msg.success));

    return msg.success;

    AROS_LIBFUNC_EXIT
} /* Request */


static VOID int_request(struct RequestActionMsg *msg,
                        struct IntuitionBase *IntuitionBase)
{
    struct Requester 	    *requester = msg->requester;
    struct Window   	    *window = msg->window;
  //ULONG   	    	     layerflags = 0;
    int     	    	     left, top, right, bottom;
  //LONG    	    	     lock;
    struct Gadget   	    *gadgets;
    int     	    	     wleft = window->LeftEdge + window->BorderLeft;
    int     	    	     wtop = window->TopEdge + window->BorderTop;
    int     	    	     wright = window->LeftEdge + window->Width - window->BorderRight- 1;
    int     	    	     wbottom = window->TopEdge + window->Height - window->BorderBottom- 1;

    if (requester->Flags & POINTREL)
    {
        if (requester == window->DMRequest)
        {
            left = window->LeftEdge + window->MouseX + requester->RelLeft;
            top = window->TopEdge + window->MouseY + requester->RelTop;
            if (left + requester->Width - 1 > wright)
                left = wright - requester->Width + 1;
            if (top + requester->Height - 1 > wbottom)
                top = wbottom - requester->Height + 1;
        }
        else
        {
            left = (wleft + wright - requester->Width) >> 1;
            top = (wtop + wbottom - requester->Height) >> 1;
            left += requester->RelLeft;
            top += requester->RelTop;
        }
    }
    else
    {
        left = wleft + requester->LeftEdge;
        top = wtop + requester->TopEdge;
    }

    if (left < wleft)
        left = wleft;
    else if (left > wright)
        left = wright;

    if (top < wtop)
        top = wtop;
    else if (top > wbottom)
        top = wbottom;

    right = left + requester->Width - 1;
    bottom = top + requester->Height - 1;

    if (right > wright)
        right = wright;

    if (bottom > wbottom)
        bottom = wbottom;

    requester->ReqLayer = NULL;
    
    if ((right >= left) && (bottom >= top))
    {
	requester->ReqLayer = CreateUpfrontHookLayer(
                	      &window->WScreen->LayerInfo
                	      , window->WScreen->RastPort.BitMap
                	      , left
                	      , top
                	      , right
                	      , bottom
                	      , (requester->Flags & SIMPLEREQ ? LAYERSIMPLE : LAYERSMART)
                	      , LAYERS_NOBACKFILL
                	      , NULL);
    }
    
    if (requester->ReqLayer)
    {
        requester->ReqLayer->Window = window;
        requester->RWindow = window;

        requester->LeftEdge = left - wleft;
        requester->TopEdge = top - wtop;

        gadgets = requester->ReqGadget;
        requester->ReqGadget = NULL;

        requester->OlderRequest = window->FirstRequest;
        window->FirstRequest = requester;
        ++window->ReqCount;
        requester->Flags |= REQACTIVE;

        AddGList(window, gadgets, 0, -1, requester);

        render_requester(requester, IntuitionBase);

        msg->success = TRUE;
    }
    else
    {
        msg->success = FALSE;
    }
}
