/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function ZipWindow()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

	AROS_LH1(void, ZipWindow,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 84, Intuition)

/*  FUNCTION
	"Zip" (move and resize) a window to the coordinates and dimensions
	the window had at the last call of ZipWindow(), or invoked via the
	zoom-gadget.

    INPUTS
	window - Which window

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	ChangeWindowBox(), MoveWindow(), SizeWindow()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/ZipWindow()
    aros_print_not_implemented ("ZipWindow");

    AROS_LIBFUNC_EXIT
} /* ZipWindow */
