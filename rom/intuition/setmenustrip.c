/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: SetMenuStrip()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH2(BOOL, SetMenuStrip,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),
	AROS_LHA(struct Menu *, menu, A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 44, Intuition)

/*  FUNCTION
	This function adds a MenuStrip to the Window, which can be invoked
	by the user after this call by pressing the right mouse button.
	Menus with no MenuItems will not be attached.

    INPUTS
	window - The window to add the MenuStrip to
	menu   - The menu to be added to the window above.

    RESULT
	TRUE if all menus have at least one menuitem.

    NOTES
	This function calculates internal values and is therfore the
	official way to add a new MenuStrip to Window.
	Always do a ClearMenuStrip() before closing the Window or adding
	another MenuStrip to the Window.

    EXAMPLE

    BUGS

    SEE ALSO
	ResetMenuStrip(), ClearMenuStrip()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    /* Calculate internal values */
#warning: TODO: calculate internal data and check for validity of struct

    /* Call the fast Method */
    ResetMenuStrip(window,menu);

return TRUE;

    AROS_LIBFUNC_EXIT
} /* SetMenuStrip */
