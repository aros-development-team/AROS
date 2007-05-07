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

        AROS_LH1(struct IntuiMessage *, GT_PostFilterIMsg,

/*  SYNOPSIS */
	AROS_LHA(struct IntuiMessage *, modimsg, A1),

/*  LOCATION */
	struct Library *, GadToolsBase, 18, GadTools)

/*  FUNCTION
	Restores an intuition message formerly changed with GT_FilterIMsg().

    INPUTS
	modimsg - The message returned from GT_FilterIMsg(). May be NULL.

    RESULT
	The original intuition message or NULL, if NULL was passed in.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GT_FilterIMsg()

    INTERNALS

    HISTORY

***************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct GT_IntuiMessage 	*gtmsg;
    struct IntuiMessage		*rc;
    
    gtmsg = (struct GT_IntuiMessage *)modimsg;
    if (gtmsg)
    {
    	/* GT_FilterIMsg (which is called by GT_GetImsg)
	   always returns an extended GadTools intuimsg */

    	rc = gtmsg->origmsg;
	
	if (gtmsg->wasalloced)
	{
	    FreeMem(gtmsg, sizeof(struct GT_IntuiMessage));
	}
    }
    else
    {
    	rc = NULL;
    }
    
    return rc;

    AROS_LIBFUNC_EXIT
    
} /* GT_PostFilterIMsg */
