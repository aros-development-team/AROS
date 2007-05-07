/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove a Bob from the gel list an the RastPort
    Lang: english
*/
#include "graphics_intern.h"
#include "gels_internal.h"

/*****************************************************************************

    NAME */
	#include <proto/graphics.h>

	AROS_LH3(void, RemIBob,

/*  SYNOPSIS */
	AROS_LHA(struct Bob *,      bob, A0),
	AROS_LHA(struct RastPort *, rp , A1),
	AROS_LHA(struct ViewPort *, vp , A2),

/*  LOCATION */
	struct GfxBase *, GfxBase, 22, Graphics)

/*  FUNCTION
	Remove a Bob immediately from RastPort and gel list.


    INPUTS
	bob - Bob
	rp  - RastPort
	vp  - ViewPort

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

	_ClearBobAndFollowClearPath(bob->BobVSprite,
	                            rp,
	                            GfxBase);
	RemVSprite(bob->BobVSprite);

	AROS_LIBFUNC_EXIT
} /* RemIBob */
