/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function MakeVPort()
    Lang: english
*/

#include <aros/debug.h>
#include <graphics/view.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH2(ULONG, MakeVPort,

/*  SYNOPSIS */
        AROS_LHA(struct View *, view, A0),
        AROS_LHA(struct ViewPort *, viewport, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 36, Graphics)

/*  FUNCTION
	Prepare a ViewPort to be displayed. Calculate all necessary internal data.
	For Amiga(tm) chipset bitmaps this includes calculating preliminary copperlists.

    INPUTS
        view     - pointer to a View structure
        viewport - pointer to a ViewPort structure
                   the viewport must have a valid pointer to a RasInfo

    RESULT
        error - Result of the operation:
	    MVP_OK         - Everything is OK, ViewPort is ready
	    MVP_NO_MEM     - There was not enough memory for internal data
	    MVP_NO_VPE     - There was no ViewPortExtra for this ViewPort and no memory to
			     allocate a temporary one.
	    MVP_NO_DSPINS  - There was not enough memory for Amiga(tm) copperlist.
	    MVP_NO_DISPLAY - The BitMap can't be displayed using specified mode (for example,
			     misaligned or wrong depth).

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
    	Currently always returns MVP_NO_DISPLAY for planar bitmaps because
	support for Amiga(tm) chipset is not implemented yet

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ViewPortExtra *vpe;
    ULONG ret = MVP_OK;
    BOOL own_vpe = FALSE;

    /* Attach a temporary ViewPortExtra if needed */
    vpe = (struct ViewPortExtra *)GfxLookUp(viewport);
    D(bug("[MakeVPort] ViewPort 0x%p, ViewPortExtra 0x%p\n", viewport, vpe));

    if (!vpe) {
        vpe = (struct ViewPortExtra *)GfxNew(VIEWPORT_EXTRA_TYPE);
	if (!vpe)
	    return MVP_NO_VPE;
	vpe->Flags = VPXF_FREE_ME;
	GfxAssociate(viewport, &vpe->n);
	own_vpe = TRUE;
    }

    /* Store bitmap object in the ViewPortExtra */
    if (IS_HIDD_BM(viewport->RasInfo->BitMap)) {
    	VPE_DATA(vpe)->Bitmap = HIDD_BM_OBJ(viewport->RasInfo->BitMap);
	D(bug("[MakeVPort] Bitmap object: 0x%p\n", VPE_DATA(vpe)->Bitmap));

	/* If we have attached colormap, we can verify that bitmap and colormap
	   modes do not differ */
	if (viewport->ColorMap) {
	    struct DisplayInfoHandle *dih = viewport->ColorMap->NormalDisplayInfo;

	    if (dih) {
	        if ((HIDD_BM_DRVDATA(viewport->RasInfo->BitMap) != dih->drv) ||
		    (HIDD_BM_HIDDMODE(viewport->RasInfo->BitMap) != dih->id)) {

		    D(bug("[MakeVPort] Bad NormalDisplayInfo\n"));
		    D(bug("[MakeVPort] Driverdata: ViewPort 0x%p, BitMap 0x%p\n", dih->drv, HIDD_BM_DRVDATA(viewport->RasInfo->BitMap)));
		    D(bug("[MakeVPort] HIDD ModeID: ViewPort 0x%p, BitMap 0x%p\n", dih->id, HIDD_BM_HIDDMODE(viewport->RasInfo->BitMap)));
		    ret = MVP_NO_DISPLAY;
		}
	    }

	    if (viewport->ColorMap->VPModeID != INVALID_ID) {
		if (GET_BM_MODEID(viewport->RasInfo->BitMap) != viewport->ColorMap->VPModeID) {

		    D(bug("[MakeVPort] Bad ModeID, ViewPort 0x%08lX, BitMap 0x%08lX\n", viewport->ColorMap->VPModeID, GET_BM_MODEID(viewport->RasInfo->BitMap)));
		    ret = MVP_NO_DISPLAY;
		}
	    }
	}
    } else {
	/* TODO: Do Amiga(tm) copperlist stuff here */
	D(bug("[MakeVPort] Planar bitmap, not supported at the moment\n"));
	ret = MVP_NO_DISPLAY;
    }

    if (ret == MVP_OK)
	/* Use ScrollVPort() in order to validate offsets */
	ScrollVPort(viewport);
    else if (own_vpe)
	GfxFree(&vpe->n);

    return ret;

    AROS_LIBFUNC_EXIT
} /* MakeVPort */
