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
#include <proto/commodities.h>

    AROS_LH1I(ULONG, CxObjType,

/*  SYNOPSIS */

	AROS_LHA(CxObj *, co, A0),

/*  LOCATION */

	struct Library *, CxBase, 10, Commodities)

/*  FUNCTION

    Obtain the type of the commodity object 'co'.

    INPUTS

    co  --  the object the type of which to get

    RESULT

    The type of the object 'co'. See <libraries/commodities.h> for the
    possible types. If 'co' is NULL, CX_INVALID is returned.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    CreateCxObj()

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    return ((co == NULL) ? CX_INVALID : CXOBJType(co));

    AROS_LIBFUNC_EXIT
} /* CxObjType */
