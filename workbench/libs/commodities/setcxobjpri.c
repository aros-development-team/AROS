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

        AROS_LH2I(LONG, SetCxObjPri,

/*  SYNOPSIS */

	AROS_LHA(CxObj *, co,  A0),
	AROS_LHA(LONG,    pri, D0),

/*  LOCATION */

	struct Library *, CxBase, 13, Commodities)

/*  FUNCTION

    Set the priority of the commodity object 'co'.

    INPUTS

    co   --  the commodity object the priority of which to change (may be
             NULL)
    pri  --  the new priority to give the object (priorities range from
             -128 to 127, a value of 0 is normal)

    RESULT

    The old priority, that is the priority of the object prior to this
    operation.

    NOTES

    EXAMPLE

    BUGS

    When using this function, the object is NOT repositioned according to
    the priority given. To achive this, remove the object from the commodity
    hierarchy using RemoveCxObj(), use SetCxPri() and reinsert it with
    EnqueueCxObj().

    SEE ALSO

    EnqueueCxObj()

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    UBYTE oldPri;

    if (co == NULL)
    {
	return 0;
    }

    oldPri = co->co_Node.ln_Pri;
    co->co_Node.ln_Pri = pri;

    return oldPri;
    
    AROS_LIBFUNC_EXIT
} /* SetCxObjPri */

