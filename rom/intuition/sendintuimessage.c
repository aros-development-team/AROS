/*
    (C) 1995-96 AROS - The Amiga Research OS
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
	struct IntuitionBase *, IntuitionBase, 121, Intuition)

/*  FUNCTION
	Private: send an IntuiMessage to an Intuition window

    INPUTS
	window - The window to which the IntuiMessage shall be sent
        imsg   - The IntuiMessage to send, which must have been allocated with
	         AllocIntuiMessage.
	
    RESULT
        none
	
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
    
    ASSERT_VALID_PTR(window);
    ASSERT_VALID_PTR(imsg);
    
    if (imsg->Qualifier & IEQUALIFIER_REPEAT)
    {
        IW(window)->num_repeatevents++;
    }
    PutMsg(window->UserPort, &imsg->ExecMessage);
    
    AROS_LIBFUNC_EXIT
}


