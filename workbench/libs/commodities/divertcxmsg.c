/*
    (C) 1997-99 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include "cxintern.h"
#include <proto/commodities.h>

    AROS_LH3I(VOID, DivertCxMsg,

/*  SYNOPSIS */

	AROS_LHA(CxMsg *, cxm      , A0),
	AROS_LHA(CxObj *, headObj  , A1),
	AROS_LHA(CxObj *, returnObj, A2),

/*  LOCATION */

	struct Library *, CxBase, 26, Commodities)

/*  FUNCTION

    Send the commodity message 'cxm' down the list of objects connected to
    'headObj' and set the destination to 'returnObj'. This means that when
    the message has travelled through the objects within the 'headObj' tree,
    the _successor_ of returnObj will receive the message.

    INPUTS

    cxm       -- the message to be diverted.
    headObj   -- the start object
    returnObj -- the successor of this object will get the message after
                 travelling through 'headObj' and friends.

    RESULT

    NOTES

    EXAMPLE

    When a filter gets a message that matches with its description, it
    sends the message down its list using:

        DivertCxMsg(msg, filter, filter);

    BUGS

    SEE ALSO

    RouteCxMsg()

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT
	
    ROUTECxMsg(cxm, (CxObj *)GetHead(&headObj->co_ObjList));
    
    if (cxm->cxm_Level >= cxm_MaxLevel)
    {
	return;
    }
    
    cxm->cxm_retObj[cxm->cxm_Level] = returnObj;
    cxm->cxm_Level++;
    
    AROS_LIBFUNC_EXIT
} /* DivertCxMsg */
