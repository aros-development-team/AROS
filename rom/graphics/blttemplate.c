/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/debug.h>
#include <proto/graphics.h>
#include <proto/oop.h>
#include "graphics_intern.h"
#include "gfxfuncsupport.h"

struct blttemplate_render_data
{
     OOP_Object *template_bm;
};

static ULONG blttemplate_render(APTR btr_data, LONG srcx, LONG srcy,
    	    	    	    	OOP_Object *dstbm_obj, OOP_Object *dst_gc,
				LONG x1, LONG y1, LONG x2, LONG y2,
			        struct GfxBase *GfxBase);
			 
/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH8(void, BltTemplate,

/*  SYNOPSIS */

	AROS_LHA(PLANEPTR		, source	, A0),
	AROS_LHA(LONG              	, xSrc		, D0),
	AROS_LHA(LONG              	, srcMod	, D1),
	AROS_LHA(struct RastPort * 	, destRP	, A1),
	AROS_LHA(LONG              	, xDest		, D2),
	AROS_LHA(LONG              	, yDest		, D3),
	AROS_LHA(LONG              	, xSize		, D4),
	AROS_LHA(LONG              	, ySize		, D5),

/*  LOCATION */
	struct GfxBase *, GfxBase, 6, Graphics)
	
/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    OOP_Object      	    	    *gc;
    struct BitMap   	    	    template_bm;
    
    struct blttemplate_render_data  btrd;
    struct Rectangle 	    	    rr;
    
    struct template_info    	    ti;
    
    struct TagItem  	    	    bm_tags[] = 
    {
    	{ aHidd_BitMap_Width	, xSize     	    	},
	{ aHidd_BitMap_Height	, ySize     	    	},
	{ aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Plane },
	{ TAG_DONE  	    	    	    	    	}
    };
    
    struct TagItem  	    	    gc_tags[] =
    {
    	{ aHidd_GC_DrawMode,	vHidd_GC_DrawMode_Copy  },
	{ TAG_DONE  	    	    	    	    	}
    };
    
    HIDDT_DrawMode  	    	    old_drmd;

    EnterFunc(bug("driver_BltTemplate(%d, %d, %d, %d, %d, %d)\n"
    	, xSrc, srcMod, xDest, yDest, xSize, ySize));
	
    FIX_GFXCOORD(xDest);
    FIX_GFXCOORD(yDest);
	
    if (!OBTAIN_DRIVERDATA(destRP, GfxBase))
    	ReturnVoid("driver_BltTemplate");
	
    gc = GetDriverData(destRP)->dd_GC;
        
    HIDD_BM_PIXTAB(&template_bm) = NULL;
    template_bm.Rows		 = ySize;
    template_bm.BytesPerRow	 = WIDTH_TO_BYTES(xSize);
    template_bm.Depth		 = 1;
    template_bm.Flags		 = BMF_AROS_HIDD;
        
    /* Create an offscreen HIDD bitmap of depth 1 to use in color expansion */
    HIDD_BM_OBJ(&template_bm) = HIDD_Gfx_NewBitMap(SDD(GfxBase)->gfxhidd, bm_tags);
    if (!HIDD_BM_OBJ(&template_bm))
    {
    	RELEASE_DRIVERDATA(destRP, GfxBase);
    	ReturnVoid("driver_BltTemplate");
    }
	
    /* Copy contents from Amiga bitmap to the offscreen HIDD bitmap */
    ti.source	 = source;
    ti.modulo	 = srcMod;
    ti.invertsrc = ((GetDrMd(destRP) & INVERSVID) ? TRUE : FALSE);

    D(bug("Copying template to HIDD offscreen bitmap\n"));

    /* Preserve state */
    OOP_GetAttr(gc, aHidd_GC_DrawMode, &old_drmd);
    OOP_SetAttrs(gc, gc_tags);

    amiga2hidd_fast( (APTR)&ti
    	, gc
    	, xSrc, 0
	, &template_bm
	, 0, 0
	, xSize, ySize
	, template_to_buf
	, GfxBase
    );
    
    /* Reset to preserved state */
    gc_tags[0].ti_Data = old_drmd;
    OOP_SetAttrs(gc, gc_tags);
    
    btrd.template_bm = HIDD_BM_OBJ(&template_bm);
    
    rr.MinX = xDest;
    rr.MinY = yDest;
    rr.MaxX = xDest + xSize - 1;
    rr.MaxY = yDest + ySize - 1;
    
    do_render_func(destRP, NULL, &rr, blttemplate_render, &btrd, FALSE, GfxBase);

    HIDD_Gfx_DisposeBitMap(SDD(GfxBase)->gfxhidd, HIDD_BM_OBJ(&template_bm));
	
    RELEASE_DRIVERDATA(destRP, GfxBase);
    
    ReturnVoid("driver_BltTemplate");
    
    AROS_LIBFUNC_EXIT
    
} /* BltTemplate */

/****************************************************************************************/

static ULONG blttemplate_render(APTR btr_data, LONG srcx, LONG srcy,
    	    	    	    	OOP_Object *dstbm_obj, OOP_Object *dst_gc,
				LONG x1, LONG y1, LONG x2, LONG y2,
			        struct GfxBase *GfxBase)
{
    struct blttemplate_render_data *btrd;
    ULONG width, height;
    
    width  = x2 - x1 + 1;
    height = y2 - y1 + 1;
    
    btrd = (struct blttemplate_render_data *)btr_data;
    
    HIDD_BM_BlitColorExpansion( dstbm_obj
    	, dst_gc
	, btrd->template_bm
	, srcx, srcy
	, x1, y1
	, width, height
     );
     
    return width * height;
    
}

/****************************************************************************************/
