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

	AROS_LH3(void, SetWindowTitles,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),
	AROS_LHA(CONST_STRPTR   , windowTitle, A1),
	AROS_LHA(CONST_STRPTR   , screenTitle, A2),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 46, Intuition)

/*  FUNCTION
	Changes the current window and/or the screen title.

    INPUTS
	window - Change the title for this window or the screen which the
		window contains.
	windowTitle - New title for the window or ((CONST_STRPTR)~0L) to keep the
		old title or NULL for no title. If you specify a string,
		this string is *NOT* copied.
	screenTitle - New title for the screen of the window or ((CONST_STRPTR)~0L)
		to keep the old title or NULL for no title. If you specify
		a title for the screen, this title will be shown when the
		window becomes active. If you specify a string, this string
		is *NOT* copied.

    RESULT
	None.

    NOTES
	You should be careful with specifying a screen title because that
	may irritate the user.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    /* Change window's title */
    if (windowTitle != (CONST_STRPTR)~0L)
    {
	window->Title = windowTitle;
	RefreshWindowFrame(window);
    }

    /* Change screen's title */
    if (!screenTitle)
	window->ScreenTitle = NULL;
    else if (screenTitle != (CONST_STRPTR)~0L)
	window->ScreenTitle = screenTitle;

    AROS_LIBFUNC_EXIT
} /* SetWindowTitles */
