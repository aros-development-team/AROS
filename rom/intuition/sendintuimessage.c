/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc:
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH2(void, SendIntuiMessage,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),
	AROS_LHA(struct IntuiMessage *, imsg, A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 151, Intuition)

/*  FUNCTION
	Private: send an IntuiMessage to an Intuition window

    INPUTS
	window - The window to which the IntuiMessage shall be sent
        imsg   - The IntuiMessage to send, which must have been allocated with
	         AllocIntuiMessage.
	
    RESULT
        none
	
    NOTES
        The caller of this function should first check himself
	whether window->UserPort is NULL. And in this case do not
	call this function at all.
	
	If inside this function the window->UserPort turns out to
	be NULL, then what happens is, that the IntuiMessage is
	immediately ReplyMessage()ed in here, just like if this was
	done by the app whose window was supposed to get the
	IntuiMessage.
	
	The protection with Forbid() is necessary, because of the
	way shared window userports are handled, when one of this
	windows is closed, where there is also just a protection with
	Forbid() when stripping those IntuiMessages from the port
	which belong to the window which is going to be closed.
	
	This function does not check whether the window to which
	the IntuiMessage is supposed to be sent, really wants to
	get the IDCMP in question, that is, whether the corresponding
	flag in window->IDCMPFLags is set.
	
	
    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    
    ASSERT_VALID_PTR(window);
    ASSERT_VALID_PTR(imsg);
    
    if (imsg->Qualifier & IEQUALIFIER_REPEAT)
    {
        IW(window)->num_repeatevents++;
    }

    Forbid();
    
    if (window->UserPort)
    {
    	if (imsg->Class == IDCMP_INTUITICKS)
	{
	    window->Flags |= WFLG_WINDOWTICKED;
	}
    	PutMsg(window->UserPort, &imsg->ExecMessage);
    }
    else
    {
    	ReplyMsg(&imsg->ExecMessage);
    }
    
    Permit();
    
    AROS_LIBFUNC_EXIT
}

