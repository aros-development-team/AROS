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
#include <proto/oop.h>

struct pattern_info
{
    PLANEPTR 	    mask;
    struct RastPort *rp;
    LONG    	    mask_bpr; /* Bytes per row */
    
    LONG    	    orig_xmin;
    LONG    	    orig_ymin;
    
    UBYTE   	    dest_depth;
    
    struct GfxBase *gfxbase;
    
};

struct bltpattern_render_data
{
     struct render_special_info rsi;
     struct pattern_info    	*pi;
};

static VOID pattern_to_buf(struct pattern_info *pi, LONG x_src, LONG y_src,
    	    	    	   LONG x_dest, LONG y_dest, ULONG xsize, ULONG ysize,
			   ULONG *buf, OOP_Object *dest_bm, HIDDT_Pixel *coltab);


static ULONG bltpattern_render(APTR bpr_data, LONG srcx, LONG srcy,
    	    	    	       OOP_Object *dstbm_obj, OOP_Object *dst_gc,
			       LONG x1, LONG y1, LONG x2, LONG y2,
			       struct GfxBase *GfxBase);

/*****************************************************************************

    NAME */
#include <clib/graphics_protos.h>

	AROS_LH7(void, BltPattern,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(PLANEPTR         , mask, A0),
	AROS_LHA(LONG             , xMin, D0),
	AROS_LHA(LONG             , yMin, D1),
	AROS_LHA(LONG             , xMax, D2),
	AROS_LHA(LONG             , yMax, D3),
	AROS_LHA(ULONG            , byteCnt, D4),

/*  LOCATION */
	struct GfxBase *, GfxBase, 52, Graphics)

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
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    if (rp->AreaPtrn)
    {
	struct pattern_info 	    	pi;
	struct bltpattern_render_data 	bprd;
	struct Rectangle    	    	rr;
	OOP_Object  	    	    	*gc;
	HIDDT_DrawMode      	    	old_drmd;
	struct TagItem      	    	gc_tags[] =
	{
	    { aHidd_GC_DrawMode ,	vHidd_GC_DrawMode_Copy	},
	    { TAG_DONE	    	    	    	    	    	}
	};

	EnterFunc(bug("driver_BltPattern(%d, %d, %d, %d, %d)\n"
    	    , xMin, yMin, xMax, yMax, byteCnt));

    	FIX_GFXCOORD(xMin);
	FIX_GFXCOORD(yMin);
	FIX_GFXCOORD(xMax);
	FIX_GFXCOORD(yMax);
	
	if (!OBTAIN_DRIVERDATA(rp, GfxBase))
    	    ReturnVoid("driver_BltPattern");

	pi.mask	= mask;
	pi.rp	= rp;
	pi.gfxbase	= GfxBase;
	pi.mask_bpr = byteCnt;
	pi.dest_depth	= GetBitMapAttr(rp->BitMap, BMA_DEPTH);

	pi.orig_xmin = xMin;
	pi.orig_ymin = yMin; 

	bprd.pi = &pi;

	gc = GetDriverData(rp)->dd_GC;

	OOP_GetAttr(gc, aHidd_GC_DrawMode, &old_drmd);
	OOP_SetAttrs(gc, gc_tags);

	rr.MinX = xMin;
	rr.MinY = yMin;
	rr.MaxX = xMax;
	rr.MaxY = yMax;

	do_render_func(rp, NULL, &rr, bltpattern_render, &bprd, TRUE, GfxBase);

	gc_tags[0].ti_Data = old_drmd;
	OOP_SetAttrs(gc, gc_tags);
	
	RELEASE_DRIVERDATA(rp, GfxBase);

    }
    else
    {
    	if (mask)
	{
	    ULONG old_drawmode = GetDrMd(rp);
	    
	    if ((old_drawmode & ~INVERSVID) == JAM2)
	    	SetDrMd(rp, JAM1 | (old_drawmode & INVERSVID));
		
	    BltTemplate(mask, 0, byteCnt, rp, xMin, yMin, xMax - xMin + 1, yMax - yMin + 1);
	    
	    SetDrMd(rp, old_drawmode);
	}
	else
	{
	    RectFill(rp, xMin, yMin, xMax, yMax);
	}
    }
    
    AROS_LIBFUNC_EXIT

} /* BltPattern */

/****************************************************************************************/

#define GfxBase pi->gfxbase

static VOID pattern_to_buf(struct pattern_info *pi, LONG x_src, LONG y_src,
    	    	    	   LONG x_dest, LONG y_dest, ULONG xsize, ULONG ysize,
			   ULONG *buf, OOP_Object *dest_bm, HIDDT_Pixel *coltab)
{

    /* x_src, y_src is the coordinates into the layer. */
    LONG    	    y;
    struct RastPort *rp;
    ULONG   	    apen;
    ULONG   	    bpen;
    ULONG   	    drmd;
    UBYTE   	    *apt;
    LONG    	    mask_x = 0, mask_y = 0;
    
    rp   = pi->rp;
    apen = GetAPen(rp);
    bpen = GetBPen(rp);
    drmd = GetDrMd(rp);
    
    apt = (UBYTE *)rp->AreaPtrn;
    
    if (pi->mask)
    {
    	mask_x = x_src; /* - pi->orig_xmin; */
	mask_y = y_src; /* - pi->orig_ymin; */
    }

    x_src += pi->orig_xmin;
    y_src += pi->orig_ymin;
    
    EnterFunc(bug("pattern_to_buf(%p, %d, %d, %d, %d, %d, %d, %p)\n"
    			, pi, x_src, y_src, x_dest, y_dest, xsize, ysize, buf ));
			

    HIDD_BM_GetImage(dest_bm
    	, (UBYTE *)buf
	, xsize * sizeof (HIDDT_Pixel)
	, x_dest, y_dest
	, xsize, ysize
	, vHidd_StdPixFmt_Native32
    );

    
    for (y = 0; y < ysize; y ++)
    {
        LONG x;
	
	for (x = 0; x < xsize; x ++)
	{
	    ULONG set_pixel;
	    ULONG pixval;
	    
	    /* Mask supplied ? */
	    if (pi->mask)
	    {
		ULONG idx, mask;


		idx = COORD_TO_BYTEIDX(x + mask_x, y + mask_y, pi->mask_bpr);
		mask = XCOORD_TO_MASK(x + mask_x);
		 
		set_pixel = pi->mask[idx] & mask;
		 
	    }
	    else set_pixel = 1UL;
		
		
	    if (set_pixel)
	    {
		if (apt)
		{
		   set_pixel = pattern_pen(rp, x + x_src, y + y_src, apen, bpen, &pixval, GfxBase);
		   if (set_pixel)
		   {
		   	D(bug(" s"));
			if (drmd & COMPLEMENT)
			{
			    pixval = ~(*buf);
			}
			else
			{
			    pixval = (coltab != NULL) ? coltab[pixval] : pixval;
			}
		    	*buf = pixval;
		   }
		    
		}
		else *buf = apen;
	    
	    } /* if (pixel should be set */
	    
	    D(bug("(%d, %d): %d", x, y, *buf));
	    buf ++;
	    
	} /* for (each column) */
	
    } /* for (each row) */
      
    ReturnVoid("pattern_to_buf");
}

#undef GfxBase

/****************************************************************************************/

static ULONG bltpattern_render(APTR bpr_data, LONG srcx, LONG srcy,
    	    	    	       OOP_Object *dstbm_obj, OOP_Object *dst_gc,
			       LONG x1, LONG y1, LONG x2, LONG y2,
			       struct GfxBase *GfxBase)
{
    struct bltpattern_render_data *bprd;
        
    ULONG width, height;
    
    bprd = (struct bltpattern_render_data *)bpr_data;
    
    width  = x2 - x1 + 1;
    height = y2 - y1 + 1;
    
    amiga2hidd_fast( (APTR)bprd->pi
    	, dst_gc
	, srcx /* bprd->rsi.layer_rel_srcx */
	, srcy /* bprd->rsi.layer_rel_srcy */
	, bprd->rsi.curbm
	, x1, y1
	, width, height
	, pattern_to_buf
	, GfxBase
    );
    
    return width * height;
    
}
/****************************************************************************************/
