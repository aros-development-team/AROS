/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"
#include "inputhandler_actions.h"

struct SetDMRequestActionMsg
{
    struct IntuiActionMsg    msg;
    struct Window   	    *window;
    struct Requester 	    *dmrequest;
    BOOL    	    	     success;
};

static VOID int_setdmrequest(struct SetDMRequestActionMsg *msg,
                             struct IntuitionBase *IntuitionBase);

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH2(BOOL, SetDMRequest,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *   , window, A0),
         AROS_LHA(struct Requester *, dmrequest, A1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 43, Intuition)

/*  FUNCTION
    Try to set the DMRequest of a window.
    A DMRequest is a requester that appears if the user double-clicks
    with the menu button.
    The new DMRequest will only be set if the old DMRequest is not in use.
    The official way to change the DMRequest is to call ClearDMRequest()
    until it returns TRUE and then call SetDMRequest().
 
 
    INPUTS
    window - The window from which the DMRequest is to be set
    dmrequest - Pointer to the requester
 
    RESULT
    TRUE if old DMRequest was not in use and therefore changed to
    the new one, or FALSE if the old DMRequest was in use and could
    not be set to the new one.
 
    NOTES
    If the DMRequest has the POINTREL flag set, the DMR will show up
    as close to the pointer as possible. The RelLeft/Top fields are
    used to fine-tune the positioning.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    ClearDMRequest(), Request()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct SetDMRequestActionMsg msg;

    SANITY_CHECKR(window,FALSE)
    SANITY_CHECKR(dmrequest,FALSE)

    msg.window    = window;
    msg.dmrequest = dmrequest;

    DoSyncAction((APTR)int_setdmrequest, &msg.msg, IntuitionBase);

    return msg.success;

    AROS_LIBFUNC_EXIT
} /* SetDMRequest */


static VOID int_setdmrequest(struct SetDMRequestActionMsg *msg,
                             struct IntuitionBase *IntuitionBase)
{
    struct Window    *window = msg->window;
    struct Requester *dmrequest = msg->dmrequest;
    LONG result;

    if (window->DMRequest && window->DMRequest->Flags & REQACTIVE)
    {
        result = FALSE;
    }
    else
    {
        window->DMRequest = dmrequest;
        result = TRUE;
    }

    msg->success = result;
}
