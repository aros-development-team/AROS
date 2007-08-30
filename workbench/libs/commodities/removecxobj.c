/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include "cxintern.h"
#include <devices/input.h>
#include <proto/exec.h>
#include <proto/commodities.h>
#include <aros/debug.h>

VOID RemoveHandler(struct CommoditiesBase *CxBase);

    AROS_LH1(VOID, RemoveCxObj,

/*  SYNOPSIS */

	AROS_LHA(CxObj *, co, A0),

/*  LOCATION */

	struct Library *, CxBase, 17, Commodities)

/*  FUNCTION

    Removes 'co' from the lists it's in. The function handles smoothly the
    cases when 'co' is NULL or haven't been inserted in a list.

    INPUTS

    co  --  the commodity object to remove (may be NULL)

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    AttachCxObj(), EnqueueCxObj(), InsertCxObj()

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    if (co == NULL)
    {
	return;
    }

    if ((co->co_Flags & COF_VALID) == 0)
    {
	return;
    }

    ObtainSemaphore(&GPB(CxBase)->cx_SignalSemaphore);

    Remove(&co->co_Node);
    co->co_Flags &= ~COF_VALID;
    
    if ((CXOBJType(co) == CX_BROKER) || (CXOBJType(co) == CX_ZERO))
    {
	if (IsListEmpty(&GPB(CxBase)->cx_BrokerList))
	{
	    RemoveHandler(GPB(CxBase));
	}
	else
	{
	    /* Tell Exchange what happened */
	    BrokerCommand(NULL, CXCMD_LIST_CHG);
	}
    }

    ReleaseSemaphore(&GPB(CxBase)->cx_SignalSemaphore);

    AROS_LIBFUNC_EXIT
} /* RemoveCxObj */


VOID RemoveHandler(struct CommoditiesBase *CxBase)
{
    if (CxBase->cx_IORequest.io_Device == NULL)
    {
	return;
    }
    
    CxBase->cx_InputMP.mp_SigTask = FindTask(NULL);
    CxBase->cx_IORequest.io_Command = IND_REMHANDLER;
    DoIO((struct IORequest *)&CxBase->cx_IORequest);
    CloseDevice((struct IORequest *)&CxBase->cx_IORequest);
    CxBase->cx_IORequest.io_Device = NULL;
    CxBase->cx_Running = FALSE;
}
