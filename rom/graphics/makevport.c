/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ViewPortExtra *vpe;
    struct HIDD_ViewPortData *vpd;
    ULONG ret = MVP_OK;
    BOOL own_vpe = FALSE;

    if (!viewport || !viewport->RasInfo || !viewport->RasInfo->BitMap)
        return MVP_NO_DISPLAY;

    /* Non-HIDD bitmaps cannot be displayed */
    if (!IS_HIDD_BM(viewport->RasInfo->BitMap)) {
        return MVP_NO_DISPLAY;
    }

    /* Attach a temporary ViewPortExtra if needed */
    vpe = (struct ViewPortExtra *)GfxLookUp(viewport);
    D(bug("[MakeVPort] ViewPort 0x%p, ViewPortExtra 0x%p\n", viewport, vpe));

    if (!vpe)
    {
        vpe = (struct ViewPortExtra *)GfxNew(VIEWPORT_EXTRA_TYPE);
	if (!vpe)
	    return MVP_NO_VPE;

	vpe->Flags = VPXF_FREE_ME;
	GfxAssociate(viewport, &vpe->n);
	own_vpe = TRUE;
    }

    /* Now make sure that ViewPortData is created */
    if (!VPE_DATA(vpe))
    	vpe->DriverData[0] = AllocMem(sizeof(struct HIDD_ViewPortData), MEMF_PUBLIC|MEMF_CLEAR);

    vpd = VPE_DATA(vpe);
    if (vpd)
    {
    	vpd->vpe = vpe;

	/*
	 * MakeVPort() can be called repeatedly on the same ViewPort.
	 * However, each time we are called, the frontmost RastInfo
	 * BitMap may be different.
	 *
	 * Updated the cached frontmost BitMap object here.
	 * We don't need to use OBTAIN_HIDD_BM(), since we can
	 * only display HIDD bitmaps (and we have verified that above).
	 */
    	vpd->Bitmap = HIDD_BM_OBJ(viewport->RasInfo->BitMap);

	D(bug("[MakeVPort] Bitmap object: 0x%p\n", vpd->Bitmap));

	if (IS_HIDD_BM(viewport->RasInfo->BitMap))
	{
	    /*
	     * If we have a colormap attached to a HIDD bitmap, we can verify
	     * that bitmap and colormap modes do not differ.
	     */
	    if (viewport->ColorMap)
	    {
	    	struct DisplayInfoHandle *dih = viewport->ColorMap->NormalDisplayInfo;

	    	if (dih)
	    	{
	            if ((HIDD_BM_DRVDATA(viewport->RasInfo->BitMap) != dih->drv) ||
		        (HIDD_BM_HIDDMODE(viewport->RasInfo->BitMap) != dih->id))
		    {

		    	D(bug("[MakeVPort] Bad NormalDisplayInfo\n"));
		    	D(bug("[MakeVPort] Driverdata: ViewPort 0x%p, BitMap 0x%p\n", dih->drv, HIDD_BM_DRVDATA(viewport->RasInfo->BitMap)));
		    	D(bug("[MakeVPort] HIDD ModeID: ViewPort 0x%p, BitMap 0x%p\n", dih->id, HIDD_BM_HIDDMODE(viewport->RasInfo->BitMap)));
		    	ret = MVP_NO_DISPLAY;
		    }
	    	}

	    	if (viewport->ColorMap->VPModeID != INVALID_ID)
	    	{
		    if (GET_BM_MODEID(viewport->RasInfo->BitMap) != viewport->ColorMap->VPModeID)
		    {
		    	D(bug("[MakeVPort] Bad ModeID, ViewPort 0x%08lX, BitMap 0x%08lX\n", viewport->ColorMap->VPModeID, GET_BM_MODEID(viewport->RasInfo->BitMap)));
		    	ret = MVP_NO_DISPLAY;
		    }
	    	}
	    }
	}

	/*
	 * Ensure that we have a bitmap object.
	 * OBTAIN_HIDD_BM() may fail on planar bitmap in low memory situation.
	 */
	if (vpd->Bitmap)
	{
	    struct monitor_driverdata *mdd = GET_VP_DRIVERDATA(viewport);

	    /*
	     * Store driverdata pointer in private ViewPortExtra field.
	     * It is needed because the caller can first free BitMap, then
	     * its ViewPort. In this case we won't be able to retrieve
	     * driver pointer from the bitmap in FreeVPortCopLists().
	     */
	    vpe->DriverData[1] = mdd;
	    ret = HIDD_Gfx_MakeViewPort(mdd->gfxhidd, vpd);
	}
	else
	    ret = MVP_NO_MEM;
    }
    else
	ret = MVP_NO_MEM;

    if (ret == MVP_OK)
	/* Use ScrollVPort() in order to validate offsets */
	ScrollVPort(viewport);
    else
    {
    	if (own_vpe)
	    GfxFree(&vpe->n);
    }

    return ret;

    AROS_LIBFUNC_EXIT
} /* MakeVPort */
