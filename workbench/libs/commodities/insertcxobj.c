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
#include <proto/commodities.h>

    AROS_LH3(VOID, InsertCxObj,

/*  SYNOPSIS */

	AROS_LHA(CxObj *, headObj, A0),
	AROS_LHA(CxObj *, co     , A1),
	AROS_LHA(CxObj *, pred   , A2),

/*  LOCATION */

	struct Library *, CxBase, 16, Commodities)

/*  FUNCTION

    Insert commodity object 'co' into the list of object connected to
    'headObj' after the object 'pred'.

    INPUTS

    headObj  -  poiter to a list of objects to which 'co' shall be inserted
    co       -  commodity object to be inserted (may be NULL)
    pred     -  the object 'co' shall be inserted after (may be NULL)

    RESULT

    If 'headObj' is NULL, the object 'co' and all objects connected to it
    are deleted. If 'co' is NULL and 'headObj' is a valid object, the
    latter's accumulated error will be adjusted to incorporate
    COERR_NULLATTACH. If 'pred' is NULL, the object will be inserted first
    in the list.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    CxObjError(), ClearCxObjError()

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    if (headObj == NULL)
    {
	DeleteCxObjAll(co);

	return;
    }

    if (co == NULL)
    {
	headObj->co_Error |= COERR_NULLATTACH;

	return;
    }
    
    ObtainSemaphore(&GPB(CxBase)->cx_SignalSemaphore);
    
    Insert((struct List *)&headObj->co_ObjList, &co->co_Node, &pred->co_Node);
    co->co_Flags |= COF_VALID;
    
    ReleaseSemaphore(&GPB(CxBase)->cx_SignalSemaphore);

    AROS_LIBFUNC_EXIT
} /* InsertCxObj */
