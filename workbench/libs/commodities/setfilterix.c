/*
    (C) 1997-98 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include "cxintern.h"
#include <libraries/commodities.h>
#include <proto/exec.h>

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
	    filter->co_Ext.co_FilterIX = ix;
	    filter->co_Error &= ~COERR_BADFILTER;
	}
	else
	{
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
