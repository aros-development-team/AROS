/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function ShowWindow()
    Lang: english
*/
#include "intuition_intern.h"
#include <proto/layers.h>

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH1(VOID, ShowWindow,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 140, Intuition)

/*  FUNCTION
        Make a window visible. This function does not bring the
        window back into the visible area of the screen but rather
        switches it into visible state.
        

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

    if (FALSE == IsWindowVisible(window))
    {
    
      msg = AllocIntuiActionMsg(AMCODE_SHOWWINDOW, window, IntuitionBase);

      if (NULL != msg)
      {
        msg->iam_ShowWindow.yesno = TRUE;
        SendIntuiActionMsg(msg, IntuitionBase);
      }
    }
    AROS_LIBFUNC_EXIT
} /* ShowWindow */
