/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PRIVATE: Indicate that a reset is imminent. 
    Lang: english
*/

#include "graphics_intern.h"
#include <proto/graphics.h>
#include <proto/oop.h>

/*****i***********************************************************************

    NAME */

	AROS_LH0(void, ShowImminentReset,

/*  SYNOPSIS */

/*  LOCATION */
	struct GfxBase *, GfxBase, 197, Graphics)

/*  FUNCTION
    	Indicates that a reset is imminent by (usually)
	blanking the screen.
	
    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
        This function can't work reliably by design. It is subject to removal.
	Don't use it, use exec reset callbacks from within display drivers
	instead.

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct monitor_driverdata *mdd;
    
    /* Call ShowImminentReset() on all drivers */
    for (mdd = CDD(GfxBase)->monitors; mdd; mdd = mdd->next)
        OOP_DoMethod(mdd->gfxhidd_orig, (OOP_Msg)&CDD(GfxBase)->hiddGfxShowImminentReset_MethodID);

    AROS_LIBFUNC_EXIT

} /* ShowImminentReset */
