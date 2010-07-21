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
	Obsolete private function. Some programs already call it, so it's
	still here in order to provide backwards compatibility. Do not use it,
	it will be removed in future!

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

    AROS_LIBFUNC_EXIT
} /* ShowImminentReset */
