/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include "cxintern.h"
#include <libraries/commodities.h>
#include <proto/exec.h>

#define DEBUG_BADFILTER(x)	x;

    AROS_LH2(VOID, SetFilterIX,

/*  SYNOPSIS */

	AROS_LHA(CxObj *, filter, A0),
	AROS_LHA(IX *   , ix    , A1),

/*  LOCATION */

	struct Library *, CxBase, 21, Commodities)

/*  FUNCTION

    Set the filter description by supplying an InputXpression.

    INPUTS

    filter  --  the commodity filter object the attributes of which to set
                (may be NULL)
    ix      --  InputXpression describing the filter

    RESULT

    The internal error field will be updated (COERR_BADFILTER) depending on
    whether the function succeeded or failed.

    NOTES

    The first field in the IX structure must be set to IX_VERSION as
    defined in <libraries/commodities.h>, to indicate which version of
    the IX structure is used.

    EXAMPLE

    BUGS

    SEE ALSO

    SetFilter(), CxObjError(), <libraries/commodities.h>

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    if (filter == NULL)
    {
	return;
    }

    ObtainSemaphore(&GPB(CxBase)->cx_SignalSemaphore);
    
    if (CXOBJType(filter) == CX_FILTER)
    {
	if (ix->ix_Version == IX_VERSION)
	{
	    CopyMem(ix, filter + 1, sizeof(IX));
	    filter->co_Ext.co_FilterIX = (APTR)(filter + 1);
	    filter->co_Error &= ~COERR_BADFILTER;
	}
	else
	{
	    DEBUG_BADFILTER(dprintf("SetFilterIX: ix_Version == %lu (should be %lu)!\n", ix->ix_Version, IX_VERSION));
	    filter->co_Error |= COERR_BADFILTER;
	}
    }
    else
    {
	filter->co_Error |= COERR_BADTYPE;
    }
    
    ReleaseSemaphore(&GPB(CxBase)->cx_SignalSemaphore);
    
    AROS_LIBFUNC_EXIT
} /* SetFilterIX */
