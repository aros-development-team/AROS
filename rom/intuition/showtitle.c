/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function ShowTitle()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH2(void, ShowTitle,

/*  SYNOPSIS */
	AROS_LHA(struct Screen *, screen, A0),
	AROS_LHA(BOOL           , ShowIt, D0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 47, Intuition)

/*  FUNCTION
	Modify SHOWTITLE flag of the screen and refresh the screen and
	its windows.
	If ShowIt is TRUE the screen's title bar will be shown in front of
	WFLG_BACKDROP windows. A value of FALSE will bring the title bar
	behind all windows.

    INPUTS

    RESULT
	None.

    NOTES
	The default of the SHOWTITLE flag for new screens is TRUE.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    if (screen->BarLayer)
    {
	struct IntuiActionMessage * msg;

	msg = AllocIntuiActionMsg(AMCODE_SCREENSHOWTITLE, NULL, IntuitionBase);

	if (NULL != msg)
	{
	    msg->iam_ShowTitle.Screen = screen;
	    msg->iam_ShowTitle.ShowIt = ShowIt;

	    SendIntuiActionMsg(msg, IntuitionBase); 
	}   
    }
    
    AROS_LIBFUNC_EXIT
    
} /* ShowTitle */
