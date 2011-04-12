/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function FreeVPortCopLists()
    Lang: english
*/

#include <aros/debug.h>
#include <graphics/copper.h>
#include <graphics/view.h>
#include <hidd/graphics.h>
#include <proto/exec.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, FreeVPortCopLists,

/*  SYNOPSIS */
	AROS_LHA(struct ViewPort *, vp, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 90, Graphics)

/*  FUNCTION
	Deallocate all memory associated with the CopList structures
	for display, color, sprite and the user copper list. The
	corresponding fields in this structure will be set to NULL.

    INPUTS
	vp - pointer to a ViewPort structure

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	graphics/view.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ViewPortExtra *vpe;

/* TODO: delegate this to the driver
    if (NULL != vp->DspIns)
    {
    	FreeCopList(vp->DspIns);
    	vp->DspIns = NULL;
    }

    if (NULL != vp->SprIns)
    {
    	FreeCopList(vp->SprIns);
    	vp->SprIns = NULL;
    }

    if (NULL != vp->ClrIns)
    {
    	FreeCopList(vp->ClrIns);
    	vp->ClrIns = NULL;
    }

    if (NULL != vp->UCopIns)
    {
    	if (NULL != vp->UCopIns->FirstCopList)
      	    FreeCopList(vp->UCopIns->FirstCopList);

    	FreeMem(vp->UCopIns, sizeof(struct UCopList));
    	vp->UCopIns = NULL;
    } */

    vpe = (struct ViewPortExtra *)GfxLookUp(vp);
    D(bug("[FreeVPortCopLists] ViewPort 0x%p, ViewPortExtra 0x%p\n", vp, vpe));
    if (vpe)
    {
	/* Free the associated DriverData */
    	struct HIDD_ViewPortData *vpd = VPE_DATA(vpe);

	if (vpd)
	{
	    if (VPE_FLAGS(vpe))
		release_cache_object(CDD(GfxBase)->planarbm_cache, vpd->Bitmap, GfxBase);

	    FreeMem(vpd, sizeof(struct HIDD_ViewPortData));

	    vpe->DriverData[0] = NULL;
	}

    	if (vpe->Flags & VPXF_FREE_ME)
    	{
    	    D(bug("[FreeVPortCopLists] Freeing temporary ViewPortExtra\n"));
    	    GfxFree(&vpe->n);
    	}
    }

    AROS_LIBFUNC_EXIT
} /* FreeVPortCopLists */
