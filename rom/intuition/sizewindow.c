/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/29 13:57:38  digulla
    Commented
    Moved common code from driver to Intuition

    Revision 1.1  1996/08/23 17:28:17  digulla
    Several new functions; some still empty.


    Desc:
    Lang: english
*/
#include "intuition_intern.h"

extern void intui_SizeWindow (struct Window * win, long dx, long dy);

/*****************************************************************************

    NAME */
	#include <clib/intuition_protos.h>

	__AROS_LH3(void, SizeWindow,

/*  SYNOPSIS */
	__AROS_LHA(struct Window *, window, A0),
	__AROS_LHA(long           , dx, D0),
	__AROS_LHA(long           , dy, D1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 48, Intuition)

/*  FUNCTION
	Modify the size of a window by the specified offsets.

    INPUTS
	window - The window to resize.
	dx - Add this to the width.
	dy - Add this to the height.

    RESULT
	None.

    NOTES
	The resize of the window may be delayed. If you depend on the
	information that is has changed size, wait for IDCMP_NEWSIZE.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    /* Call the driver before changing the window */
    intui_SizeWindow (window, dx, dy);

    /* Adjust the window's attributes */
    window->Width += dx;
    window->Height += dy;

    __AROS_FUNC_EXIT
} /* SizeWindow */
