/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/intuition.h>
#include "gadtools_intern.h"

/*********************************************************************

    NAME */
#include <proto/gadtools.h>
#include <intuition/intuition.h>

        AROS_LH1(void, GT_BeginRefresh,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, win, A0),

/*  LOCATION */
	struct Library *, GadToolsBase, 15, GadTools)

/*  FUNCTION
	Refreshes the given window. This function must be used instead
	of BeginRefresh(), if gadtools-gadgets are used. When you are
	finished with refreshing the window, you must call GT_EndRefresh().

    INPUTS
	win - window to refresh

    RESULT

    NOTES
	Due to the internal structure of gadtools, it is not possible
	to use WFLG_NOCAREREFRESH with windows, which use gadtools-
	gadgets.
	You should simple rendering functions between GT_BeginRefresh() and
	GT_EndRefresh() only. Do not render or change any gadgets.

    EXAMPLE
	case IDCMP_REFRESHWINDOW:
	    GT_BeginRefresh(mywin);
	    GT_EndRefresh(mywin, TRUE);

    BUGS

    SEE ALSO
	GT_EndRefresh(), intuition.library/BeginRefresh()

    INTERNALS

    HISTORY

***************************************************************************/
{
    AROS_LIBFUNC_INIT

    BeginRefresh(win);

    AROS_LIBFUNC_EXIT
    
} /* GT_BeginRefresh */
