/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function LoadView()
    Lang: english
*/

#include <aros/debug.h>
#include <graphics/view.h>
#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include <exec/memory.h>
#include <graphics/rastport.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <oop/oop.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(void, LoadView,

/*  SYNOPSIS */
        AROS_LHA(struct View *, view, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 37, Graphics)

/*  FUNCTION

    INPUTS
        view - pointer to the View structure which contains the pointer to the
               constructed coprocessor instructions list, or NULL

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	Currently AROS doesn't have actual view & viewport implementation. For the
	user this means that you can't drag screens. Only one screen is visible, and
	it's the frontmost one.
	The frontmost Intuition screen corresponds to the first ViewPort in the view
	(see intuition/rethinkdisplay.c)

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    #warning THIS IS NOT THREADSAFE

    /* To make this threadsafe we have to lock
       all gfx access in all the rendering calls
    */
    struct BitMap *bitmap;
    OOP_Object      	*cmap, *pf;
    HIDDT_ColorModel 	colmod;
    OOP_Object      	*fb;
    BOOL    	    	ok = FALSE;
    ULONG   	    	showflags = 0;

    /* Take the first ViewPort from the view. Its bitmap will be the frontmost one.
       If NULL is passed as a view, we simply clear the display. */
    bitmap = view ? view->ViewPort->RasInfo->BitMap : NULL;

    D(bug("LoadView(): displaying bitmap 0x%p\n", bitmap));
    //if (bitmap && (BMF_DISPLAYABLE != (bitmap->Flags & BMF_DISPLAYABLE)))
    if (bitmap && (!(bitmap->Flags & BMF_AROS_HIDD) || !(HIDD_BM_FLAGS(bitmap) & HIDD_BMF_SCREEN_BITMAP)))
    {
    	D(bug("!!! LoadView(): TRYING TO SET NON-DISPLAYABLE BITMAP !!!\n"));
	return;
    }

    if ( SDD(GfxBase)->frontbm == bitmap)
    {
    	D(bug("!!!!!!!!!!!!!!! SHOWING BITMAP %p TWICE !!!!!!!!!!!\n", bitmap));
	return;
    }

    if (bitmap)
    {
    	showflags |= fHidd_Gfx_Show_CopyBack;
    }

    fb = HIDD_Gfx_Show(SDD(GfxBase)->gfxhidd, (bitmap ? HIDD_BM_OBJ(bitmap) : NULL), showflags);

    if (NULL == fb)
    {
    	D(bug("!!! LoadView(): HIDD_Gfx_Show() FAILED !!!\n"));
    }
    else
    {
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
	    IPTR width, height;

	    /* Insert the framebuffer in its place */
	    OOP_GetAttr(fb, aHidd_BitMap_ColorMap, (IPTR *)&cmap);
	    OOP_GetAttr(fb, aHidd_BitMap_PixFmt, (IPTR *)&pf);
	    OOP_GetAttr(pf, aHidd_PixFmt_ColorModel, &colmod);

	    HIDD_BM_OBJ(bitmap)	= fb;
	    HIDD_BM_COLMOD(bitmap)	= colmod;
	    HIDD_BM_COLMAP(bitmap)	= cmap;

#if 1 /* CHECKME! */
            OOP_GetAttr(SDD(GfxBase)->bm_bak, aHidd_BitMap_Width, &width);
    	    OOP_GetAttr(SDD(GfxBase)->bm_bak, aHidd_BitMap_Height, &height);

	    HIDD_BM_UpdateRect(fb, 0, 0, width, height);
#endif
    	    
	}
	Permit();
    }

    AROS_LIBFUNC_EXIT
} /* LoadView */
