/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#define AROS_ALMOST_COMPATIBLE

#include "cxintern.h"
#include <exec/memory.h>
#include <libraries/commodities.h>
#include <proto/exec.h>
#include <proto/commodities.h>

    AROS_LH1(LONG, CopyBrokerList,

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

    This function is present in AmigaOS too but undocumented.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    LONG               count = 0;
    struct BrokerCopy *brokerCopy;
    CxObj             *broker;

    FreeBrokerList(CopyofList);

    ObtainSemaphore(&GPB(CxBase)->cx_SignalSemaphore);

    ForeachNode(&GPB(CxBase)->cx_BrokerList, broker)
    {
	if (CxObjType(broker) == CX_ZERO)
	{
	    break;
	}

	brokerCopy = AllocVec(sizeof(struct BrokerCopy), MEMF_PUBLIC | MEMF_CLEAR);

	if (brokerCopy == NULL)
	{
	    break;
	}

	/* Copy the broker name */
	CopyMem(broker->co_Ext.co_BExt, brokerCopy->bc_Name, sizeof(*broker->co_Ext.co_BExt));
	brokerCopy->bc_Flags = broker->co_Flags;
	/* Point the node name to the broker name */
	brokerCopy->bc_Node.ln_Name = brokerCopy->bc_Name;
	

        AddTail(CopyofList, &brokerCopy->bc_Node);
        count++;
    }

    ReleaseSemaphore(&GPB(CxBase)->cx_SignalSemaphore);

    return count;

    AROS_LIBFUNC_EXIT
} /* CopyBrokerList */
