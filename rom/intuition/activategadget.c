/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function ActivateGadget()
    Lang: english
*/
#include "intuition_intern.h"
#include "inputhandler.h"
#include "gadgets.h"

#include <proto/exec.h>

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

    struct IntuiActionMessage 	*msg;
    BOOL 			success = FALSE;
    
    if (gadget && (window || requester))
    {
    	if (!(gadget->Activation & GACT_ACTIVEGADGET))
	{
	    if (((gadget->GadgetType & GTYP_GTYPEMASK) == GTYP_CUSTOMGADGET) ||
        	((gadget->GadgetType & GTYP_GTYPEMASK) == GTYP_STRGADGET))
	    {
		msg = AllocIntuiActionMsg(AMCODE_ACTIVATEGADGET, window, IntuitionBase);

		if (NULL != msg)
		{
		    msg->iam_ActivateGadget.Gadget = gadget;

		    SetSignal(0,SIGF_INTUITION);
		    SendIntuiActionMsg(msg, IntuitionBase);
		    Wait(SIGF_INTUITION);

		    success = (BOOL)msg->Code;

		    FreeIntuiActionMsg(msg, IntuitionBase);
		    
		}   
		
	    }
	    
	} /* if (!(gadget->Activation & GACT_ACTIVEGADGET)) */
	else
	{
	    success = TRUE;
	}
	
    } /* if (gadget && (window || requester)) */
    
    return success;

    AROS_LIBFUNC_EXIT
    
} /* ActivateGadget */
