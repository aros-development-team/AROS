/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Change position and size of a window
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/intuition_protos.h>

	AROS_LH5(void, ChangeWindowBox,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),
	AROS_LHA(LONG           , left, D0),
	AROS_LHA(LONG           , top, D1),
	AROS_LHA(LONG           , width, D2),
	AROS_LHA(LONG           , height, D3),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 81, Intuition)

/*  FUNCTION
	Set the new position and size of a window in one call.

    INPUTS
	window - Change this window
	left, top - New position
	width, height - New size

    RESULT

    NOTES
	This call is deferred. Wait() for IDCMP_CHANGEWINDOW if your
	program depends on the new size.

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
    
    msg = AllocIntuiActionMsg(AMCODE_CHANGEWINDOWBOX, window, IntuitionBase);
    if (NULL != msg)
    {
	msg->iam_ChangeWindowBox.Left   = left;
	msg->iam_ChangeWindowBox.Top    = top;
	msg->iam_ChangeWindowBox.Width  = width;
	msg->iam_ChangeWindowBox.Height = height;
	
	SendIntuiActionMsg(msg, IntuitionBase);
    }

    AROS_LIBFUNC_EXIT
    
} /* ChangeWindowBox */
