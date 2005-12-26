/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function AddVSprite()
    Lang: english
*/
#include <graphics/gels.h>
#include <graphics/rastport.h>
#include "graphics_intern.h"
#include "gels_internal.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(void, AddVSprite,

/*  SYNOPSIS */
	AROS_LHA(struct VSprite *, vs, A0),
	AROS_LHA(struct RastPort *, rp, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 17, Graphics)

/*  FUNCTION
	The VSprite is linked into the current gel list using it's
        y and x coordinates. The VSprite's flags are set up.

    INPUTS
	vs = pointer to VSprite to be linked into gel list
	rp = pointer to RastPort that has an initilized GelsInfo linked
             to it (see InitGels()).

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	InitGels()  graphics/gels.h graphics/rastport.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
	AROS_LIBFUNC_INIT
	AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

	struct VSprite * CurVSprite;
	/* the Y-coordinate is most significant! */
	LONG Koord = JOIN_XY_COORDS(vs->X, vs->Y);

	/* Reset the Flags for this VSprite and set OldX/Y */
	vs -> Flags &= 0xFF;
	vs -> OldY = vs -> Y;
	vs -> OldX = vs -> X;

	CurVSprite = rp->GelsInfo->gelHead;

	/* look for the appropriate place to insert the VSprite into the
	   list of VSprites which is connected to the GelsInfo which was
	   previously found in the rastport */

	while (JOIN_XY_COORDS(CurVSprite->NextVSprite->X, CurVSprite->NextVSprite->Y) < Koord)
		CurVSprite = CurVSprite->NextVSprite;

	/* insert the new VSprite *after* CurVSprite */

	CurVSprite -> NextVSprite -> PrevVSprite = vs;
	vs -> NextVSprite = CurVSprite -> NextVSprite;
	vs -> PrevVSprite = CurVSprite;
	CurVSprite -> NextVSprite = vs;

	/*
	 * Create he IntVSprite structure for improved handling of
	 * the VSprite ImageData.
	 */
	vs -> IntVSprite = _CreateIntVSprite(vs, rp, GfxBase);

	AROS_LIBFUNC_EXIT

} /* AddVSprite */
