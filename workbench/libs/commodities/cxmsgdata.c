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

    AROS_LH1I(APTR, CxMsgData,

/*  SYNOPSIS */

	AROS_LHA(CxMsg *, cxm, A0),

/*  LOCATION */

	struct Library *, CxBase, 24, Commodities)

/*  FUNCTION

    Get the data of a commodities message. Messages can be sent from
    both sender object and custom object. In the first case the data is
    no longer valid after you replied to the message.

    INPUTS

    cxm  -  the message the data of which is to be retrieved (may be NULL).

    RESULT

    A pointer to the message's data or NULL if message was NULL. The type
    of the data depends on the type of the message.

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

    return (cxm == NULL) ? NULL : cxm->cxm_Data;

    AROS_LIBFUNC_EXIT
} /* CxMsgData */
