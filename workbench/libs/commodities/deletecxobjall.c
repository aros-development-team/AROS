/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include "cxintern.h"
#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/commodities.h>

VOID RecRem(CxObj *, struct Library *CxBase);

    AROS_LH1(VOID, DeleteCxObjAll,

/*  SYNOPSIS */

	AROS_LHA(CxObj *, co, A0),

/*  LOCATION */

	struct Library *, CxBase, 9, Commodities)

/*  FUNCTION

    Delete object and and all objects connected to commodity object 'co'.
    Handy for instances like when you are shutting down your commodity.
    To remove your commodity tree, just DeleteCxObjAll(YourBroker).

    INPUTS

    co  --  the object in question (may be NULL)

    RESULT

    NOTES

    The handle 'co' is invalid after the operation.

    EXAMPLE

    BUGS

    SEE ALSO

    DeleteCxObj()

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    if (co == NULL)
    {
	return;
    }

    RemoveCxObj(co);
    RecRem((CxObj *)GetHead(&co->co_ObjList), CxBase);
    FreeCxStructure(co, CX_OBJECT, CxBase);

    AROS_LIBFUNC_EXIT
} /* DeleteCxObjAll */


VOID RecRem(CxObj *obj, struct Library *CxBase)
{
    CxObj *next;
    
    while (obj != NULL)
    {
        RecRem((CxObj *)GetHead(&obj->co_ObjList), CxBase);        

        next = (CxObj *)GetSucc(obj);        
	FreeCxStructure(obj, CX_OBJECT, CxBase);
        obj = next;
        
    }
}
