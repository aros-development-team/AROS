/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/29 13:33:30  digulla
    Moved common code from driver to Intuition
    More docs

    Revision 1.1  1996/08/23 17:28:17  digulla
    Several new functions; some still empty.


    Desc:
    Lang: english
*/
#include "intuition_intern.h"

extern intui_ActivateWindow (struct Window *);

/*****************************************************************************

    NAME */
	#include <clib/intuition_protos.h>

	__AROS_LH1(void, ActivateWindow,

/*  SYNOPSIS */
	__AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 75, Intuition)

/*  FUNCTION
	Activates the specified window. The window gets the focus
	and all further input it sent to that window. If the window
	requested it, it will get a IDCMP_ACTIVEWINDOW message.

    INPUTS
	window - The window to activate

    RESULT
	None.

    NOTES
	If the user has an autopointer tool (sunmouse), the call will
	succeed, but the tool will deactivate the window right after
	this function has activated it. It is no good idea to try to
	prevent this by waiting for IDCMP_INACTIVEWINDOW and activating
	the window again since that will produce an anoying flicker and
	it will slow down the computer a lot.

    EXAMPLE

    BUGS

    SEE ALSO
	ModiyIDCMP(), OpenWindow(), CloseWindow()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    intui_ActivateWindow (window);

    /* This comes _after_ intui_ActivateWindow() because the driver
	might want to deactivate the old window first */
    IntuitionBase->ActiveWindow = window;

    __AROS_FUNC_EXIT
} /* ActivateWindow */
