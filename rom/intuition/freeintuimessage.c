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

	AROS_LH1(void, FreeIntuiMessage,

/*  SYNOPSIS */
	AROS_LHA(struct IntuiMessage *, imsg, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 112, Intuition)

/*  FUNCTION
	Private to AROS: free an IntuiMessage previously allocated
	with AllocIntuiMessage.

    INPUTS
	imsg - The IntuiMessage. May be NULL.

    RESULT
	none

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    
    ASSERT_VALID_PTR_OR_NULL(imsg);
    
    if (imsg)
    {
        FreeMem(imsg, sizeof(struct IntIntuiMessage));
    }

    AROS_LIBFUNC_EXIT
}


