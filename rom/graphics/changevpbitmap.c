/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function ChangeVPBitMap()
    Lang: english
*/

#include <aros/debug.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <hidd/graphics.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH3(void, ChangeVPBitMap,

/*  SYNOPSIS */
        AROS_LHA(struct ViewPort *, vp, A0),
        AROS_LHA(struct BitMap *, bm, A1),
        AROS_LHA(struct DBufInfo *, db, A2),

/*  LOCATION */
        struct GfxBase *, GfxBase, 157, Graphics)

/*  FUNCTION

    INPUTS
        vp - pointer to a viewport
        bm - pointer to a BitMap structure. This BitMap structure must be of
             the same layout as the one attached to the viewport
             (same depth, alignment and BytesPerRow)
        db - pointer to a DBufInfo

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ViewPort *vp2;

    /* This is a very basic implementation. Screen refresh is completely not in sync with VBlank.
       The main problem here is that AROS completely misses VBlank interrupt. */

    /* Insert a new bitmap and rebuild the viewport */
    vp->RasInfo->BitMap = bm;
    MakeVPort(GfxBase->ActiView, vp);

    ObtainSemaphore(GfxBase->ActiViewCprSemaphore);

    if (GfxBase->ActiView)
    {
    	for (vp2 = GfxBase->ActiView->ViewPort; vp2; vp2 = vp2->Next)
    	{
    	    /* First check if the updated ViewPort is currently on display */
	    if (vp2 == vp)
	    {
		if (!(vp2->Modes & VP_HIDE))
		{
	    	    /*
	    	     * If yes, we need to rebuild driver's display. Look up the driver
	    	     * and its ViewPorts chain and redisplay it.
	    	     * We don't check against vpd == NULL because we already know
	    	     * there's at least one ViewPort (our one) in the chain.
	    	     */
	    	    struct monitor_driverdata *mdd = GET_VP_DRIVERDATA(vp);
	    	    struct HIDD_ViewPortData *vpd = driver_FindViewPorts(GfxBase->ActiView, mdd, GfxBase);

		    HIDD_Gfx_PrepareViewPorts(mdd->gfxhidd, vpd, GfxBase->ActiView);
	    	    driver_LoadViewPorts(vpd, GfxBase->ActiView, mdd, GfxBase);
	    	}

	    	break;
	    }
	}
    }

    ReleaseSemaphore(GfxBase->ActiViewCprSemaphore);

    /* Reply both messages - the displayed bitmap has been swapped */
    ReplyMsg(&db->dbi_SafeMessage);
    ReplyMsg(&db->dbi_DispMessage);

    AROS_LIBFUNC_EXIT
} /* ChangeVPBitMap */
