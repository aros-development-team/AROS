/*
    (C) 1997-2001 AROS - The Amiga Research OS
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

    brokerList  --  List of commodity brokers (a list of struct BrokerCopy
                    nodes).

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    cxintern.h

    INTERNALS

    Private function.

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    struct BrokerCopy *brokerCopy;

    while ((brokerCopy = (CxObj *)RemHead(brokerList)) != NULL)
    {
        FreeVec(brokerCopy);
    }

    AROS_LIBFUNC_EXIT
} /* FreeBrokerList */
