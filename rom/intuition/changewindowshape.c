/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function ChangeWindowShape()
    Lang: english
*/
#include "intuition_intern.h"
#include <proto/layers.h>

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH3(struct Region *, ChangeWindowShape,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),
    	AROS_LHA(struct Region *, newshape, A1),
	AROS_LHA(struct Hook *, callback, A2),
	
/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 143, Intuition)

/*  FUNCTION
        Make a window invisible. 

    INPUTS
	window - The window to affect. 

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

    struct IntuiActionMessage * msg;
    struct Region   	      * retval = NULL;
    
    ASSERT_VALID_PTR(window);
    
    if (IS_GZZWINDOW(window)) return NULL;
    
    msg = AllocIntuiActionMsg(AMCODE_CHANGEWINDOWSHAPE, window, IntuitionBase);
    if (NULL != msg)
    {
        msg->iam_ChangeWindowShape.shape    = newshape;
	msg->iam_ChangeWindowShape.callback = callback;
        
	SetSignal(0, SIGF_INTUITION);
	SendIntuiActionMsg(msg, IntuitionBase);
	Wait(SIGF_INTUITION);
	
	retval = msg->iam_ChangeWindowShape.shape;
	
	FreeIntuiActionMsg(msg, IntuitionBase);
    }

    return retval;
    
    AROS_LIBFUNC_EXIT
    
} /* ChangeWindowShape */
