/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include "cxintern.h"
#include <proto/exec.h>
#include <proto/commodities.h>
#include <libraries/commodities.h>

    AROS_LH2(VOID, EnqueueCxObj,

/*  SYNOPSIS */

	AROS_LHA(CxObj *, headObj, A0),
	AROS_LHA(CxObj *, co     , A1),

/*  LOCATION */

	struct Library *, CxBase, 15, Commodities)

/*  FUNCTION

    Insert commodity object 'co' into the list of objects connected to
    'headObj' according to the priority of 'co'. (The priority of an object
    can be set by the function SetCxObjPri().)

    INPUTS

    headObj - the object to which 'co' shall be inserted.
    co      - a pointer to a commodity object

    RESULT

    If 'headObj' is NULL, the object 'co' and all objects connected to it
    are deleted. If 'co' is NULL and 'headObj' is a valid object, the
    latter's accumulated error will be adjusted to incorporate
    COERR_NULLATTACH.

    NOTES

    For nodes with equal priority, this function inserts object like within
    a FIFO queue.

    EXAMPLE

    BUGS

    SEE ALSO

    SetCxObjPri(), CxObjError(), ClearCxObjError(),
    <libraries/commodities.h>

    INTERNALS

    HISTORY

*****************************************************************************/

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

    Enqueue((struct List *)&headObj->co_ObjList, &co->co_Node);
    co->co_Flags |= COF_VALID;

    ReleaseSemaphore(&GPB(CxBase)->cx_SignalSemaphore);

    AROS_LIBFUNC_EXIT
} /* EnqueueCxObj */
