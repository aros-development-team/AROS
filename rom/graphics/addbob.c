/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function AddBob()
    Lang: english
*/
#include <graphics/gels.h>
#include <graphics/rastport.h>
#include "graphics_intern.h"
#include "gels_internal.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(void, AddBob,

/*  SYNOPSIS */
	AROS_LHA(struct Bob *, bob, A0),
	AROS_LHA(struct RastPort *, rp, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 16, Graphics)

/*  FUNCTION
	The Bob is linked into the current gel list via AddVSprite.
	The Bob's flags are set up.

    INPUTS
	Bob = pointer to Bob to be added to gel list
	rp  = pointer to RastPort that has an initilized GelsInfo linked
	      to it (see InitGels()).

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	InitGels() AddVSprite() graphics/rastport.h graphics/gels.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
	AROS_LIBFUNC_INIT
	AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

	if (NULL != rp->GelsInfo) {

		/*
		 * Check whether the head and tail VSprite
		 * were alone before. If so, then give them
		 * a IntVSprite structure now.
		 */
		if (rp->GelsInfo->gelHead->NextVSprite ==
		    rp->GelsInfo->gelTail) {
			_CreateIntVSprite(rp->GelsInfo->gelHead, NULL, GfxBase);
			_CreateIntVSprite(rp->GelsInfo->gelTail, NULL, GfxBase);
		}
		bob->Flags &= 0x00FF;
		bob->Flags |= BWAITING;
		AddVSprite (bob->BobVSprite, rp);
	}
	
	AROS_LIBFUNC_EXIT
} /* AddBob */
