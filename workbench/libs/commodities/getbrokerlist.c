/*
    (C) 1997-99 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#define AROS_ALMOST_COMPATIBLE

#include "cxintern.h"
#include <libraries/commodities.h>
#include <proto/exec.h>
#include <proto/commodities.h>

    AROS_LH1(LONG, GetBrokerList,

/*  SYNOPSIS */

	AROS_LHA(struct List *, CopyofList, A0),

/*  LOCATION */

	struct Library *, CxBase, 31, Commodities)

/*  FUNCTION

    Get a copy of the internal list of brokers.

    INPUTS

    CopyofList -- pointer to a list

    RESULT

    The number of brokers in the list. The elements of the input list will
    be deallocated.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    Private function.

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    LONG         count = 0;
    CxObj       *te2;
    CxObj       *tempObj;

    FreeBrokerList(CopyofList);

    ObtainSemaphore(&GPB(CxBase)->cx_SignalSemaphore);

    ForeachNode(&GPB(CxBase)->cx_BrokerList, (struct Node *)tempObj)
    {
	APTR  ptrSave;

	if (CxObjType(tempObj) == CX_ZERO)
	{
	    break;
	}

	if (!(te2 = (CxObj *)AllocCxStructure(CX_OBJECT, CX_BROKER, CxBase)))
	{
	    break;
	}

	ptrSave = te2->co_Ext.co_BExt;
	CopyMem(tempObj, te2, sizeof(CxObj));
	te2->co_Ext.co_BExt = ptrSave;
        *te2->co_Ext.co_BExt = *tempObj->co_Ext.co_BExt;

        AddTail(CopyofList, (struct Node *)te2);
        count++;
    }

    ReleaseSemaphore(&GPB(CxBase)->cx_SignalSemaphore);

    return count;

    AROS_LIBFUNC_EXIT
} /* GetBrokerList */
