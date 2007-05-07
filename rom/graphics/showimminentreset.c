/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PRIVATE: Indicate that a reset is imminent. 
    Lang: english
*/

#include "graphics_intern.h"
#include <proto/graphics.h>
#include <proto/oop.h>

/*****************************************************************************

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

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
	
    OOP_DoMethod(SDD(GfxBase)->gfxhidd_orig, (OOP_Msg)&SDD(GfxBase)->hiddGfxShowImminentReset_MethodID);
    
    AROS_LIBFUNC_EXIT
	
} /* ShowImminentReset */
