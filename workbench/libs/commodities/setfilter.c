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
#include <proto/commodities.h>

    AROS_LH2(VOID, SetFilter,

/*  SYNOPSIS */

	AROS_LHA(CxObj *, filter, A0),
	AROS_LHA(STRPTR,  text,   A1),

/*  LOCATION */

	struct Library *, CxBase, 20, Commodities)

/*  FUNCTION

    Make 'filter' match events of the type specified in 'text'.

    INPUTS

    filter  -  the commodity filter the matching conditions of which to set
    text    -  description telling what to filter

    RESULT

    The internal error field will be updated (COERR_BADFILTER) according to
    the success or failure of the operation.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    SetFilterIX(), CxObjError()

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
	if (ParseIX(text, filter->co_Ext.co_FilterIX) == 0)
	{
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
} /* SetFilter */
