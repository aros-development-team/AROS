/*
    (C) 1997-99 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include "cxintern.h"
#include "proto/exec.h"

    AROS_LH1(VOID, FreeBrokerList,

/*  SYNOPSIS */

	AROS_LHA(struct List *, brokerList, A0),

/*  LOCATION */

	struct Library *, CxBase, 32, Commodities)

/*  FUNCTION

    Free the list of brokers obtained by calling GetBrokerList.

    INPUTS

    brokerList - List of commodity brokers.

    RESULT

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

    CxObj *broker;

    while ((broker = (CxObj *)RemHead(brokerList)) != NULL)
    {
        FreeCxStructure(broker, CX_OBJECT, CxBase);
    }

    AROS_LIBFUNC_EXIT
} /* FreeBrokerList */
