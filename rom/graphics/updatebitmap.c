/*
    Copyright © 2011-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Update specified region of the bitmap, taking software composition into account.
    	  Private function for cybergraphics.library support.
    Lang: english
*/

#include "graphics_intern.h"
#include "compositor_driver.h"
#include "gfxfuncsupport.h"

#include <hidd/graphics.h>

AROS_LH5(void, UpdateBitMap,
	 AROS_LHA(struct BitMap *, bitmap, A0),
	 AROS_LHA(UWORD, x     , D0),
	 AROS_LHA(UWORD, y     , D1),
	 AROS_LHA(UWORD, width , D2),
	 AROS_LHA(UWORD, height, D3),
	 struct GfxBase *, GfxBase, 201, Graphics)
{
    AROS_LIBFUNC_INIT

    /* This function must be called only on HIDD bitmaps */
    update_bitmap(bitmap, HIDD_BM_OBJ(bitmap), x, y, width, height, GfxBase);

    AROS_LIBFUNC_EXIT
}

void update_bitmap(struct BitMap *bitmap, OOP_Object *bm, UWORD x, UWORD y, UWORD width, UWORD height, struct GfxBase *GfxBase)
{
    struct monitor_driverdata *mdd = GET_BM_DRIVERDATA(bitmap);

    if (mdd->compositor)
    	compositor_UpdateBitMap(mdd->compositor, bm, x, y, width, height, GfxBase);
    else
    	HIDD_BM_UpdateRect(bm, x, y, width, height);
}
