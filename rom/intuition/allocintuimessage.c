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

	AROS_LH1(struct IntuiMessage *, AllocIntuiMessage,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 96, Intuition)

/*  FUNCTION
	Private to AROS: allocate an IntuiMessage. IntuiMessage->Window
	will be set to window by this function.

    INPUTS
	window - The window to which the IntuiMessage will be sent

    RESULT
	an allocated IntuiMessage structure. Must be freed with FreeIntuiMessage

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
    
    struct IntuiMessage *msg;
    
    ASSERT_VALID_PTR(window);
    
    if ((msg = AllocMem(sizeof(struct IntIntuiMessage), MEMF_PUBLIC | MEMF_CLEAR)))
    {
        msg->ExecMessage.mn_Node.ln_Type = NT_MESSAGE;
	msg->ExecMessage.mn_Length       = sizeof(struct ExtIntuiMessage);
	msg->ExecMessage.mn_ReplyPort    = window->WindowPort;
	
        msg->IDCMPWindow 	         = window;
    } 

    return msg;
    
    AROS_LIBFUNC_EXIT
}


