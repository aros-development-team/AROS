/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intuition function SetDMRequest
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH2(BOOL, SetDMRequest,

/*  SYNOPSIS */
	AROS_LHA(struct Window *   , window, A0),
	AROS_LHA(struct Requester *, dmrequest, A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 43, Intuition)

/*  FUNCTION
	Try to set the DMRequest of a window.
	A DMRequest is a requester that appears if the user double-clicks
	with the menu button.
	The new DMRequest will only be set if the old DMRequest is not in use.
	The official way to change the DMRequest is to call ClearDMRequest()
	until it returns TRUE and then call SetDMRequest().
	

    INPUTS
	window - The window from which the DMRequest is to be set
	dmrequest - Pointer to the requester

    RESULT
	TRUE if old DMRequest was not in use and therefore changed to
	the new one, or FALSE if the old DMRequest was in use and could
	not be set to the new one.

    NOTES
	If the DMRequest has the POINTREL flag set, the DMR will show up
	as close to the pointer as possible. The RelLeft/Top fields are
	used to fine-tune the positioning.

    EXAMPLE

    BUGS

    SEE ALSO
	ClearDMRequest(), Request()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

#warning TODO: Write intuition/SetDMRequest()
    aros_print_not_implemented ("SetDMRequest");

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* SetDMRequest */
