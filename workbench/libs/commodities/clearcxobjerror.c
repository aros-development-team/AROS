/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include "cxintern.h"
#include "proto/commodities.h"

    AROS_LH1I(VOID, ClearCxObjError,

/*  SYNOPSIS */

	AROS_LHA(CxObj *, co, A0),

/*  LOCATION */

	struct Library *, CxBase, 12, Commodities)

/*  FUNCTION

    Clears the accumulated error of the commodity object 'co'.

    INPUTS

    co  --  the object in question

    RESULT

    NOTES

    An error of type COERR_BADFILTER should not be cleared as this tells
    commodities that the filter in question is all right, and this is not
    what you want. Set a correct filter (via SetFilter() or SetFilterIX())
    or don't change the error value.
    
    EXAMPLE

    BUGS

    SEE ALSO

    CxObjError()

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    if (co != NULL)
    {
	co->co_Error = 0;
    }

    AROS_LIBFUNC_EXIT
} /* ClearCxObject */
