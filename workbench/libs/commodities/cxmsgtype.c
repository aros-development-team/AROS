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

    AROS_LH1I(ULONG, CxMsgType,

/*  SYNOPSIS */

	AROS_LHA(CxMsg *, cxm, A0),

/*  LOCATION */

	struct Library *, CxBase, 23, Commodities)

/*  FUNCTION

    Obtain the type of the commodity message 'cxm'.

    INPUTS

    cxm - The message the type of which is to be determined (may NOT be
          NULL).

    RESULT

    The type of 'cxm'. The available types of commodity messages is defined
    in <libraries/commodities.h>.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    return cxm->cxm_Type;

    AROS_LIBFUNC_EXIT
} /* CxMsgType */
