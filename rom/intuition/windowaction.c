/*
    Copyright  2003-2013, The AROS Development Team. All rights reserved.
    Copyright  2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <utility/tagitem.h>
#include <intuition/extensions.h>

#include "intuition_intern.h"
#include "inputhandler_support.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH3(void, WindowAction,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),
	AROS_LHA(ULONG, action, D0),
	AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 157, Intuition)

/*  FUNCTION
	Perform an asynchronous action on a window that is not controlled
	by the caller task.

	This function is safe even when the window is destroyed by the owner
	during the call.

    INPUTS
	window - a window to act upon
	action - a requested action code
	tags   - additional parameters, depending on the action

	Currently defined actions are:

	  WAC_SENDIDCMPCLOSE - send an IDCMP_CLOSEWINDOW message.

    RESULT
	None.

    NOTES
	This function is compatible with MorphOS.

	The requested action is executed asynchronously; the function actually
	returns before it is complete.

    EXAMPLE

    BUGS
	At the moment only WAC_SENDIDCMPCLOSE action is implemented.

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    /* TODO: Write intuition/WindowAction() */

    switch (action)
    {
        case WAC_SENDIDCMPCLOSE:
            ih_fire_intuimessage(window, IDCMP_CLOSEWINDOW, 0, 0, IntuitionBase);
            break;

        default:
            aros_print_not_implemented ("WindowAction");
            break;
    }

    AROS_LIBFUNC_EXIT
} /* WindowAction */
