/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#define DEBUG 0
#include <aros/debug.h>

#include "cxintern.h"
#include <libraries/commodities.h>
#include <proto/commodities.h>

    AROS_LH2(LONG, ActivateCxObj,

/*  SYNOPSIS */

	AROS_LHA(CxObj *, co,   A0),
	AROS_LHA(LONG,    true, D0),

/*  LOCATION */

	struct Library *, CxBase, 7, Commodities)

/*  FUNCTION

    Activates/deactivates a given commodity object. (An inactive object
    doesn't perform its function on its input - it just passes it on to 
    the next object.) The activation depends on the value of 'true'; if
    it's TRUE the object is activated, if it's FALSE it's deactivated.
        All objects are created in the active state except for brokers;
    remember to activate your broker when you have linked your other
    objects to it.

    INPUTS

    co   - a pointer to a commodity object
    true - boolean telling whether the object should be activated or
           deactivated

    RESULT

    The activation state of the object prior to the operation. (0 is
    also returned if 'co' was NULL.)

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    CxBroker()

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    LONG temp;

    D(bug("Enter ActivateCxObj(cxobj = %p, true = %d)\n", co, true));
    
    if (co == NULL)
    {
	return 0;
    }

    temp = (co->co_Flags & COF_ACTIVE);

    if (true)
    {
	co->co_Flags |= COF_ACTIVE;
    }
    else
    {
	co->co_Flags &= ~COF_ACTIVE;
    }

    if (co->co_Node.ln_Type == CX_BROKER)
    {
	CxNotify(NULL, CXCMD_LIST_CHG);
    }

    return temp;

    AROS_LIBFUNC_EXIT
} /* ActivateCxObject */
