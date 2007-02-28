/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private graphics function for allocating screen bitmaps
    Lang: english
*/
#include "graphics_intern.h"
#include <exec/memory.h>
#include <graphics/rastport.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include "gfxfuncsupport.h"
#include "dispinfo.h"

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH1(struct BitMap * , AllocScreenBitMap,

/*  SYNOPSIS */
	AROS_LHA(ULONG, modeid, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 182, Graphics)

/*  FUNCTION
	Allocates a bitmap for use with a screen opened by OpenScreen()

    INPUTS
	modeid - the DisplayID of the screen to create

    RESULT
    	bitmap - pointer to the newly created bitmap.

    NOTES
	This function is private and AROS specific.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)
    
    struct BitMap   *nbm = NULL;
    HIDDT_ModeID    hiddmode;
    
    /* First get the the gfxmode for this modeid */
    hiddmode = get_hiddmode_for_amigamodeid(modeid, GfxBase);
    
    if (vHidd_ModeID_Invalid != hiddmode)
    {
	/* Create the bitmap from the hidd mode */
	OOP_Object *sync, *pf;
	
	if (HIDD_Gfx_GetMode(SDD(GfxBase)->gfxhidd, hiddmode, &sync, &pf))
	{
	    ULONG width, height, depth;
	    
	    OOP_GetAttr(sync, aHidd_Sync_HDisp,	&width);
	    OOP_GetAttr(sync, aHidd_Sync_VDisp,	&height);
	    OOP_GetAttr(pf,   aHidd_PixFmt_Depth,	&depth);
	    
	    /* Hack: a negative depth indicates to AllocBitMap, that
	       the friend bitmap param actually is the hiddmode */
	       
	    nbm = AllocBitMap(width, height, -((LONG)depth), HIDD_BMF_SCREEN_BITMAP | BMF_DISPLAYABLE, (struct BitMap *)hiddmode);
	}
    }
    
    return nbm;

    AROS_LIBFUNC_EXIT
    
} /* AllocScreenBitMap */
