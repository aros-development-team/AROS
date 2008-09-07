/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: (AROS only) Graphics function AndRectRect()
    Lang: english
*/
#include <aros/debug.h>
#include "graphics_intern.h"
#include <graphics/regions.h>
#include <clib/macros.h>
#include "intregions.h"

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH3(BOOL, AndRectRect,

/*  SYNOPSIS */
    	AROS_LHA(struct Rectangle *, rect1, A0),
	AROS_LHA(struct Rectangle *, rect2, A1),
	AROS_LHA(struct Rectangle *, intersect, A2),

/*  LOCATION */
	struct GfxBase *, GfxBase, 193, Graphics)

/*  FUNCTION
    	Calculate the intersection rectangle between the
	given Rectangle rect1 and the given Rectangle rect2
	leaving the result in intersect (if intersect != NULL).

    INPUTS
    	rect1 - pointer to 1st Rectangle
	rect2 - pointer to 2nd Rectangle
	intersect -> pointer to rectangle which will hold result. May be NULL.

    RESULT
	TRUE if rect1 and rect2 do intersect. In this case intersect (unless NULL)
	will contain the intersection rectangle.

	FALSE if rect1 and rect2 do not overlap. "intersect" will
	then be left unchanged.

    NOTES
	This function does not exist in AmigaOS.

    EXAMPLE

    BUGS

    SEE ALSO
	AndRectRegion(), AndRegionRegion()

    INTERNALS

    HISTORY
	15-12-2000  stegerg implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ASSERT_VALID_PTR(rect1);
    ASSERT_VALID_PTR(rect2);
    ASSERT_VALID_PTR_OR_NULL(intersect);

    if (intersect)
        return _AndRectRect(rect1, rect2, intersect);
    else
        return overlap(*rect1, *rect2);

    AROS_LIBFUNC_EXIT

} /* AndRectRect */
