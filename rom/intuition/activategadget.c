/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"
#include "inputhandler_actions.h"
#include "inputhandler.h"
#include "gadgets.h"

#include <proto/exec.h>

struct ActivateGadgetActionMsg
{
    struct IntuiActionMsg    msg;
    struct Window   	    *window;
    struct Requester	    *requester;
    struct Gadget   	    *gadget;
    BOOL    	    	     success;
};

#define DEBUG_ACTIVATEGADGET(x) ;

static VOID int_activategadget(struct ActivateGadgetActionMsg *msg,
                               struct IntuitionBase *IntuitionBase);

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH3(BOOL, ActivateGadget,

         /*  SYNOPSIS */
         AROS_LHA(struct Gadget *   , gadget, A0),
         AROS_LHA(struct Window *   , window, A1),
         AROS_LHA(struct Requester *, requester, A2),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 77, Intuition)

/*  FUNCTION
    Activates the specified gadget.
 
    INPUTS
    gadget - The gadget to activate
    window - The window which contains the gadget
    requester - The requester which contains the gadget or
        NULL if it is not a requester gadget
 
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

    BOOL success = FALSE;

    DEBUG_ACTIVATEGADGET(dprintf("ActivateGadget: Gadget 0x%lx Window 0x%lx Req 0x%lx\n",
                                 gadget, window, requester));

    if (gadget && (window || requester))
    {
        if (1)
        {
            if ((((gadget->GadgetType & GTYP_GTYPEMASK) == GTYP_CUSTOMGADGET) && (!(gadget->Activation & GACT_ACTIVEGADGET))) ||
                ((gadget->GadgetType & GTYP_GTYPEMASK) == GTYP_STRGADGET))
            {
                struct ActivateGadgetActionMsg msg;
		
                msg.window    = window;
                msg.requester = requester;
                msg.gadget    = gadget;

                DEBUG_ACTIVATEGADGET(dprintf("ActivateGadget: send gadget activation msg\n"));

                DoSyncAction((APTR)int_activategadget, &msg.msg, IntuitionBase);

                success = msg.success;
            }
            else
            {
                DEBUG_ACTIVATEGADGET(dprintf("ActivateGadget: no Custom/Stringgadget\n"));
            }

        } /* if (!(gadget->Activation & GACT_ACTIVEGADGET)) */
        else
        {
            DEBUG_ACTIVATEGADGET(dprintf("ActivateGadget: Gadget Activation bit already set\n"));
            success = TRUE;
        }

    } /* if (gadget && (window || requester)) */
    else
    {
        DEBUG_ACTIVATEGADGET(dprintf("ActivateGadget: No Gadget/Window/Requester\n"));
    }
    DEBUG_ACTIVATEGADGET(dprintf("ActivateGadget: Success %d\n",
                                 success));

    return success;

    AROS_LIBFUNC_EXIT

} /* ActivateGadget */


static VOID int_activategadget(struct ActivateGadgetActionMsg *msg,
                               struct IntuitionBase *IntuitionBase)
{
    struct IIHData   *iihdata = (struct IIHData *)GetPrivIBase(IntuitionBase)->InputHandler->is_Data;
    struct Window    *window = msg->window;
    struct Requester *req = msg->requester;
    struct Gadget    *gadget = msg->gadget;

    DEBUG_ACTIVATEGADGET(dprintf("int_ActivateGadget: Gadget 0x%lx Window 0x%lx Req 0x%lx\n",
                	 gadget,
                	 window,
                	 req));

    msg->success = FALSE;

    /* Activate gadget only if no other gadget is
       actually active and the gadget to be
       activated is in the actual active window
       and the gadget is not disabled */

    DEBUG_ACTIVATEGADGET(dprintf("int_ActivateGadget: activeGadget 0x%lx\n",
                    iihdata->ActiveGadget));

    if ((iihdata->ActiveGadget == NULL) &&
        (IntuitionBase->ActiveWindow == window) &&
        ((gadget->Flags & GFLG_DISABLED) == 0))
    {
        msg->success = (DoActivateGadget(window, req, gadget, IntuitionBase) != NULL);
    }
}
