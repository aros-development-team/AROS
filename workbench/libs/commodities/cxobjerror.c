/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include "cxintern.h"
#include <proto/commodities.h>

    AROS_LH1I(LONG, CxObjError,

/*  SYNOPSIS */

	AROS_LHA(CxObj *, co, A0),

/*  LOCATION */

	struct Library *, CxBase, 11, Commodities)

/*  FUNCTION

    Obtain the ackumulated error of commodity object 'co'.

    INPUTS

    co  -  the object the error of which to get

    RESULT

    The ackumulated error of the object 'co'. See <libraries/commodities.h>
    for the possible errors.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    ClearCxObjError()

    INTERNALS

    There seems to be some kind of RKRM error here. The RKRM says that
    COERR_ISNULL is returned if 'co' is NULL. COERR_ISNULL is furthermore
    defined as 1 in <libraries/commodities.h>. However, in the RESULTS part
    is says "error - the accumulated error, or 0 if 'co' is NULL". As the
    commodities.library I have (39.6 I believe) returns 0, I do the same
    below.

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    return (co == NULL) ? 0 : co->co_Error;

    AROS_LIBFUNC_EXIT
} /* CxObjError */
