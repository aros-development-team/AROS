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

    AROS_LH2(VOID, AttachCxObj,

/*  SYNOPSIS */

	AROS_LHA(CxObj *, headObj, A0),
	AROS_LHA(CxObj *, co,      A1),

/*  LOCATION */

	struct Library *, CxBase, 14, Commodities)

/*  FUNCTION

    Add commodity object 'co' last in the list of objects of object
    'headObj'.

    INPUTS

    headObj - pointer to a list of commodity objects
    co      - the object to add to the list


    RESULT

    If 'headObj' is NULL the entire tree of objects pointed to by 'co'
    is deleted. If 'co' is NULL, this is recorded in the error field of
    'headObj'.

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
    
    AddTail((struct List *)&headObj->co_ObjList, &co->co_Node);
    co->co_Flags |= COF_VALID;
    
    ReleaseSemaphore(&GPB(CxBase)->cx_SignalSemaphore);
    
    AROS_LIBFUNC_EXIT
} /* AttachCxObject */
