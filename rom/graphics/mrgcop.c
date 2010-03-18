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
    struct BitMap *bitmap = NULL;
    OOP_Object      	*cmap, *pf;
    HIDDT_ColorModel 	colmod;
    OOP_Object      	*fb;

    /* Take the first visibme ViewPort from the view. Its bitmap will be the frontmost one.
       If NULL is passed as a view, or there are no visible ViewPorts, we simply clear the display
       (bitmap is NULL). */
    
    if (view) {
        for (vp = view->ViewPort; vp; vp = vp->Next) {
            if (!(vp->Modes & VP_HIDE)) {
	        bitmap = vp->RasInfo->BitMap;
	        break;
	    }
	}
    }

    D(bug("MrgCop(): displaying bitmap 0x%p\n", bitmap));
    if (bitmap && (!(bitmap->Flags & BMF_AROS_HIDD) || !(HIDD_BM_FLAGS(bitmap) & HIDD_BMF_SCREEN_BITMAP)))
    {
    	D(bug("!!! MrgCop(): TRYING TO SET NON-DISPLAYABLE BITMAP !!!\n"));
	return MCOP_NO_MEM;
    }

    #warning THIS IS NOT THREADSAFE

    /* To make this threadsafe we have to lock
       all gfx access in all the rendering calls
    */

    /* If the given view is not a current one, we don't apply changes.
       In this case we just tell the result of view validation. */
    if (GfxBase->ActiView != view)
        return bitmap ? MCOP_OK : MCOP_NOP;

    /* TODO: Here we should somehow pass view->ViewPort->ColorMap->SpriteBase_Even to the driver. May be add
       a parameter to Show() method? */

    if ( SDD(GfxBase)->frontbm == bitmap)
    {
    	D(bug("!!!!!!!!!!!!!!! SHOWING BITMAP %p TWICE !!!!!!!!!!!\n", bitmap));
	return MCOP_OK;
    }

    fb = HIDD_Gfx_Show(SDD(GfxBase)->gfxhidd, (bitmap ? HIDD_BM_OBJ(bitmap) : NULL), fHidd_Gfx_Show_CopyBack);

    if (NULL == fb)
    {
	if (bitmap) {
    	    D(bug("!!! MrgCop(): HIDD_Gfx_Show() FAILED !!!\n"));
	    return MCOP_NO_MEM;
	} else
	    return MCOP_OK;
    }
    else
    {
    	IPTR width, height;

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

	return MCOP_OK;
    }

    AROS_LIBFUNC_EXIT
} /* MrgCop */
