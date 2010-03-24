/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function MrgCop()
    Lang: english
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <graphics/rastport.h>
#include <oop/oop.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(ULONG, MrgCop,

/*  SYNOPSIS */
        AROS_LHA(struct View *, view, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 35, Graphics)

/*  FUNCTION
        Merge together the display, color, sprite and user coprocessor
		instructions into a single coprocessor instruction stream.
		
    INPUTS
        view - a pointer to the view structure whos coprocessor instructions
		       are to be merged.

    RESULT
        error - ULONG error value indicating either lack of memory to build the system copper lists,
		        or that MrgCop() has no work to do - ie there where no viewPorts in the list.

    NOTES
        Pre-v39 AmigaOS returns void.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	AROS currently doesn't run on Amiga hardware, so we don't work with real copperlists. However
	we try to behave as if we work with them. So if the view is set as active, we immediately apply
	all changes. Otherwise we just perform some validation.

	Currently AROS doesn't have support for screens composition. Only one screen is visible, and
	it's the frontmost one.	The frontmost Intuition screen corresponds to the first ViewPort in
	the view (see intuition/rethinkdisplay.c)

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ViewPort *vp;
    struct ViewPortExtra *vpe;
    struct BitMap *bitmap = NULL;
    OOP_Object      	*cmap, *pf;
    HIDDT_ColorModel 	colmod;
    OOP_Object      	*fb;

    if (GfxBase->ActiView != view)
        return MCOP_OK;

    #warning THIS IS NOT THREADSAFE

    /* To make this threadsafe we have to lock
       all gfx access in all the rendering calls
    */

    /* Remove the whole bitmap stack from the driver */
    D(bug("[MrgCop] Clearing display...\n"));
    /* This Show() returns non-NULL only if we use framebuffer */
    fb = HIDD_Gfx_Show(SDD(GfxBase)->gfxhidd, NULL, fHidd_Gfx_Show_CopyBack);

    /* Show new bitmaps */
    if (view) {
        for (vp = view->ViewPort; vp; vp = vp->Next) {
            if (!(vp->Modes & VP_HIDE)) {
	        /* We don't check against vpe == NULL because MakeVPort() has already took care about this */
	        vpe = GfxLookUp(vp);
	        D(bug("[MrgCop] Showing bitmap object 0x%p\n", VPE_BITMAP(vpe)));
		/* Here we supply a bitmap object, so we will get NULL only on failure */
	        if (!HIDD_Gfx_Show(SDD(GfxBase)->gfxhidd, VPE_BITMAP(vpe), fHidd_Gfx_Show_CopyBack))
		    return MCOP_NO_MEM;
		/* If we use framebuffer, we can't perform screen composition. At least currently.
		   In this case we show only one frontmost bitmap. */
		if (fb) {
	            bitmap = vp->RasInfo->BitMap;
	            break;
		}
	    }
	}
    }

    
    if (fb) {
    	IPTR width, height;

	D(bug("[MrgCop] Replacing framebuffer\n"));
	Forbid();

	 /* Set this as the active screen */
    	if (NULL != SDD(GfxBase)->frontbm)
	{
    	    struct BitMap *oldbm;
    	    /* Put back the old values into the old bitmap */
	    oldbm = SDD(GfxBase)->frontbm;
	    HIDD_BM_OBJ(oldbm)		= SDD(GfxBase)->bm_bak;
	    HIDD_BM_COLMOD(oldbm)	= SDD(GfxBase)->colmod_bak;
	    HIDD_BM_COLMAP(oldbm)	= SDD(GfxBase)->colmap_bak;
	}

	SDD(GfxBase)->frontbm		= bitmap;
	SDD(GfxBase)->bm_bak		= bitmap ? HIDD_BM_OBJ(bitmap) : NULL;
	SDD(GfxBase)->colmod_bak	= bitmap ? HIDD_BM_COLMOD(bitmap) : NULL;
	SDD(GfxBase)->colmap_bak	= bitmap ? HIDD_BM_COLMAP(bitmap) : NULL;

	if (bitmap)
	{
	    /* Insert the framebuffer in its place */
	    OOP_GetAttr(fb, aHidd_BitMap_ColorMap, (IPTR *)&cmap);
	    OOP_GetAttr(fb, aHidd_BitMap_PixFmt, (IPTR *)&pf);
	    OOP_GetAttr(pf, aHidd_PixFmt_ColorModel, &colmod);

	    HIDD_BM_OBJ(bitmap)	= fb;
	    HIDD_BM_COLMOD(bitmap)	= colmod;
	    HIDD_BM_COLMAP(bitmap)	= cmap;
	}

        OOP_GetAttr(fb, aHidd_BitMap_Width, &width);
    	OOP_GetAttr(fb, aHidd_BitMap_Height, &height);

	HIDD_BM_UpdateRect(fb, 0, 0, width, height);
	Permit();
    }

    return MCOP_OK;

    AROS_LIBFUNC_EXIT
} /* MrgCop */
