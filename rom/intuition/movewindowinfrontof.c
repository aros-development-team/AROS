/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function MoveWindowInFrontOf()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/intuition_protos.h>

	AROS_LH2(void, MoveWindowInFrontOf,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),
	AROS_LHA(struct Window *, behindwindow, A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 80, Intuition)

/*  FUNCTION
	Arrange the relative depth of a window.

    INPUTS
	window - the window to reposition
	behindwindow - the window the other one will be brought in front of

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	WindowToFront(), WindowToBack(), layers.library/MoveLayerInFrontOf()

    INTERNALS
	Uses layers.library/MoveLayerInFrontOf().

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct IntuiActionMessage * msg;
    
    msg = AllocIntuiActionMsg(AMCODE_MOVEWINDOWINFRONTOF, window, IntuitionBase);
 
    if (NULL != msg)
    {
	msg->iam_MoveWindowInFrontOf.BehindWindow = behindwindow;

	SendIntuiActionMsg(msg, IntuitionBase); 
    }   

    AROS_LIBFUNC_EXIT
    
} /* MoveWindowInFrontOf */
