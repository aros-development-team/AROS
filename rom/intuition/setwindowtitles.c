/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.8  1999/12/19 19:27:16  nlorentz
    Now calls RefreshWindowFrame()

    Revision 1.7  1998/10/20 16:46:05  hkiel
    Amiga Research OS

    Revision 1.6  1997/01/27 00:36:43  ldp
    Polish

    Revision 1.5  1996/12/10 14:00:09  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.4  1996/11/08 11:28:04  aros
    All OS function use now Amiga types

    Moved intuition-driver protos to intuition_intern.h

    Revision 1.3  1996/10/24 15:51:25  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.2  1996/08/29 13:57:38  digulla
    Commented
    Moved common code from driver to Intuition

    Revision 1.1  1996/08/23 17:28:18  digulla
    Several new functions; some still empty.


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
	AROS_LHA(UBYTE         *, windowTitle, A1),
	AROS_LHA(UBYTE         *, screenTitle, A2),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 46, Intuition)

/*  FUNCTION
	Changes the current window and/or the screen title.

    INPUTS
	window - Change the title for this window or the screen which the
		window contains.
	windowTitle - New title for the window or ((UBYTE *)~0L) to keep the
		old title or NULL for no title. If you specify a string,
		this string is *NOT* copied.
	screenTitle - New title for the screen of the window or ((UBYTE *)~0L)
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

    /* Call driver before changing the window to allow it to examine
	the old values. */
    intui_SetWindowTitles (window, windowTitle, screenTitle);

    /* Change window's title */
    if (!windowTitle)
	window->Title = NULL;
    else if (windowTitle != (UBYTE *)~0L)
	window->Title = windowTitle;
	
    RefreshWindowFrame(window);

    /* Change screen's title */
    if (!screenTitle)
	window->ScreenTitle = NULL;
    else if (screenTitle != (UBYTE *)~0L)
	window->ScreenTitle = screenTitle;

    AROS_LIBFUNC_EXIT
} /* SetWindowTitles */
