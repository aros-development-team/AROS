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

    AROS_LH2I(VOID, RouteCxMsg,

/*  SYNOPSIS */

	AROS_LHA(CxMsg *, cxm, A0),
	AROS_LHA(CxObj *, co , A1),

/*  LOCATION */

	struct Library *, CxBase, 27, Commodities)

/*  FUNCTION

    Set the next destination of a commodity message to be 'co'.
    ('co' must be a valid commodity object and linked in to the commodities
    hierarchy.)

    INPUTS

    cxm  -  the commodity message to route (may NOT be NULL)
    co   -  the commodity object to route the message to (may NOT be NULL)

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    DivertCxMsg()

    INTERNALS

    HISTORY

******************************************************************************/

{
   AROS_LIBFUNC_INIT

   ROUTECxMsg(cxm, co);

   AROS_LIBFUNC_EXIT
} /* RouteCxMsg */
