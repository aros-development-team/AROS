/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/11/08 11:28:00  aros
    All OS function use now Amiga types

    Moved intuition-driver protos to intuition_intern.h

    Revision 1.3  1996/10/24 15:51:17  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.2  1996/08/29 13:33:30  digulla
    Moved common code from driver to Intuition
    More docs

    Revision 1.1  1996/08/23 17:28:17  digulla
    Several new functions; some still empty.


    Desc:
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/intuition_protos.h>

	AROS_LH1(void, ActivateWindow,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    intui_ActivateWindow (window);

    /* This comes _after_ intui_ActivateWindow() because the driver
	might want to deactivate the old window first */
    IntuitionBase->ActiveWindow = window;

    AROS_LIBFUNC_EXIT
} /* ActivateWindow */
