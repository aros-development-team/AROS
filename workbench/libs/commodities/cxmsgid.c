/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include "cxintern.h"
#include <proto/commodities.h>

    AROS_LH1I(LONG, CxMsgID,

/*  SYNOPSIS */

	AROS_LHA(CxMsg *, cxm, A0),

/*  LOCATION */

	struct Library *, CxBase, 25, Commodities)

/*  FUNCTION

    Retrieve the ID of a certain CxMsg 'cxm'. (IDs for sender and custom
    objects are supplied by the user when the objects are created.)

    INPUTS

    cxm     --  the message in question (may NOT be NULL)

    RESULT

    The ID of the message 'cxm'. If not specified by the application the ID
    is 0.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    cx_lib/CxSender(), cx_lib/CxCustom()

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    return cxm->cxm_ID;

    AROS_LIBFUNC_EXIT
} /* CxMsgID */

