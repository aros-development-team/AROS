/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include "cxintern.h"
#include <libraries/commodities.h>
#include <devices/inputevent.h>
#include <proto/commodities.h>

#include <aros/debug.h>

    AROS_LH2(BOOL, MatchIX,

/*  SYNOPSIS */

	AROS_LHA(struct InputEvent *, event, A0),
	AROS_LHA(IX *               , ix   , A1),

/*  LOCATION */

	struct Library *, CxBase, 34, Commodities)

/*  FUNCTION

    Check if an input event matches an input expression.

    INPUTS

    event  --  the input event to match against the input expression
    ix     --  the input expression

    RESULT

    TRUE if the input event matches the input expression, FALSE otherwise.

    NOTES

    Applications don't normally need this function as filter objects take
    care of the event filtering. However, this function is in some cases
    nice to have.

    EXAMPLE

    BUGS

    SEE ALSO

    <libraries/commodities.h>, ParseIX()

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    UWORD temp = 0;
    UWORD qual;
    
    if (ix->ix_Class == IECLASS_NULL)
    {
	return TRUE;
    }
    
    if (event->ie_Class != ix->ix_Class)
    {
	return FALSE;
    }

    //    kprintf("Code: ie %i ix: %i\n", event->ie_Code, ix->ix_Code);
    
    if (((ix->ix_Code ^ event->ie_Code) & ix->ix_CodeMask) != 0)
    {
	return FALSE;
    }
    
    temp = event->ie_Qualifier;

    if ((qual = ix->ix_QualSame) != 0)
    {
	if ((qual & IXSYM_SHIFT) != 0)
	{
	    if ((temp & IXSYM_SHIFTMASK) != 0)
	    {
		temp |= IXSYM_SHIFTMASK;
	    }
	}

	if ((qual & IXSYM_CAPS) != 0)
	{
	    if ((temp & IXSYM_CAPSMASK) != 0)
	    {
		temp |= IXSYM_CAPSMASK;
	    }
	}

	if ((qual & IXSYM_ALT) != 0)
	{
	    if ((temp & IXSYM_ALTMASK) != 0)
	    {
	        temp |= IXSYM_ALTMASK;
	    }
	}
    }

    if (((temp ^ ix->ix_Qualifier) & ix->ix_QualMask) == 0)
    {
	return TRUE;
    }
    else
    {
	return FALSE;
    }

    AROS_LIBFUNC_EXIT
} /* MatchIX */
