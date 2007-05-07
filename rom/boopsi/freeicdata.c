/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Support function for icclass and gadgetclass
    Lang: english
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include "intern.h"

/*****i***********************************************************************

    NAME */

	AROS_LH1(void, FreeICData,

/*  SYNOPSIS */
	AROS_LHA(struct ICData *, icdata, A0),

/*  LOCATION */
	struct Library *, BOOPSIBase, 15, BOOPSI)

/*  FUNCTION
	This private function will free the data that belongs to an object
	of the type ICCLASS. It is primarily in as a private function for
	the benefit of intuition.library's gadgetclass implementation, 
	which includes an icclass of its own.

    INPUTS
	    icdata	-	The address of a struct ICData
	
    RESULT
	    The data associated will have been freed (including the TagList).
    
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    icdata->ic_LoopCounter = 0UL;
    
    if(icdata->ic_CloneTags)
    {
	FreeTagItems(icdata->ic_CloneTags);
	icdata->ic_CloneTags = NULL;
    }

    AROS_LIBFUNC_EXIT
} /* FreeICData */
