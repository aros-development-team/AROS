/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$    $Log

    Desc: Graphics function EraseRect()
    Lang: english
*/
#include <aros/debug.h>
#include "graphics_intern.h"
#include <graphics/rastport.h>
#include <aros/asmcall.h>
#include <utility/hooks.h>
#include <proto/oop.h>
#include "gfxfuncsupport.h"

struct layerhookmsg
{
    struct Layer    *Layer;
    WORD    	    MinX, MinY, MaxX, MaxY;
    LONG    	    OffsetX, OffsetY;
};

struct eraserect_render_data
{
    struct render_special_info  rsi;
    struct RastPort 	    	*origrp;
    struct RastPort 	    	*fakerp;
};

static void calllayerhook(struct Hook *h, struct RastPort *rp, 
                    	  struct layerhookmsg *msg, struct GfxBase * GfxBase);


static ULONG eraserect_render(APTR err_data, LONG srcx, LONG srcy,
    	    	    	      OOP_Object *dstbm_obj, OOP_Object *gc,
			      LONG x1, LONG y1, LONG x2, LONG y2,
			      struct GfxBase *GfxBase);

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH5(void, EraseRect,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(LONG             , xMin, D0),
	AROS_LHA(LONG             , yMin, D1),
	AROS_LHA(LONG             , xMax, D2),
	AROS_LHA(LONG             , yMax, D3),

/*  LOCATION */
	struct GfxBase *, GfxBase, 135, Graphics)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    struct eraserect_render_data errd;
    struct Rectangle 	    	 rr;
    
    EnterFunc(bug("EraseRect(%d, %d, %d, %d)\n", xMin, yMin, xMax, yMax));

    if (!OBTAIN_DRIVERDATA(rp, GfxBase))
    	ReturnVoid("EraseRect(No driverdata)");
	
    errd.origrp = rp;
    errd.fakerp = NULL;
    
    rr.MinX = xMin;
    rr.MinY = yMin;
    rr.MaxX = xMax;
    rr.MaxY = yMax;
    
    do_render_func(rp, NULL, &rr, eraserect_render, &errd, TRUE, GfxBase);
    
    if (NULL != errd.fakerp)
    	FreeRastPort(errd.fakerp);
   
    RELEASE_DRIVERDATA(rp, GfxBase);
    
    ReturnVoid("EraseRect");

    AROS_LIBFUNC_EXIT

} /* EraseRect */

/****************************************************************************************/

static void calllayerhook(struct Hook *h, struct RastPort *rp, 
                	  struct layerhookmsg *msg, struct GfxBase * GfxBase)
{
    struct BitMap   *bm = rp->BitMap;
    OOP_Object      *gc;
    
    if (!OBTAIN_DRIVERDATA(rp, GfxBase)) 
    	return;
		
    gc = GetDriverData(rp)->dd_GC;
    
    if(h == LAYERS_BACKFILL)
    {
	OOP_Object *bm_obj;
	
	bm_obj = OBTAIN_HIDD_BM(bm);
	if (NULL != bm_obj)
	{
	
	     HIDDT_DrawMode old_drmd;
	     HIDDT_Pixel old_fg;
	     
	     struct TagItem gc_tags[] =
	     {
	     	{aHidd_GC_Foreground, 0UL   	    	    },
		{aHidd_GC_DrawMode  , vHidd_GC_DrawMode_Copy},
		{TAG_DONE   	    	    	    	    }
	     };
	     
	     OOP_GetAttr(gc, aHidd_GC_DrawMode,   &old_drmd);
	     OOP_GetAttr(gc, aHidd_GC_Foreground, &old_fg);

	     gc_tags[0].ti_Data = BM_PIXEL(rp->BitMap, 0);

	     OOP_SetAttrs(gc, gc_tags);
		    
	     /* Cliprect not obscured, so we may render directly into the display */
	     HIDD_BM_FillRect(bm_obj
	     	, gc
		, msg->MinX, msg->MinY
		, msg->MaxX, msg->MaxY
	     );
	     
	     gc_tags[0].ti_Data = old_fg;
	     gc_tags[1].ti_Data = old_drmd;
	     
	     OOP_SetAttrs(gc, gc_tags);
	     
	     RELEASE_HIDD_BM(bm_obj, bm);
	     
	} /* if (NULL != bm_obj)*/
	
    } /* if(h == LAYERS_BACKFILL) */
    else if (h != LAYERS_NOBACKFILL)
    {
	/* Call user specified hook */
	AROS_UFC3(void, h->h_Entry,
	    AROS_UFCA(struct Hook *,         h,    A0),
	    AROS_UFCA(struct RastPort *,     rp,   A2),
	    AROS_UFCA(struct layerhookmsg *, msg, A1)
	);
    }
    
    RELEASE_DRIVERDATA(rp, GfxBase);
}

/****************************************************************************************/

static ULONG eraserect_render(APTR err_data, LONG srcx, LONG srcy,
    	    	    	      OOP_Object *dstbm_obj, OOP_Object *gc,
			      LONG x1, LONG y1, LONG x2, LONG y2,
			      struct GfxBase *GfxBase)
{

    struct layerhookmsg     	    msg;
    struct eraserect_render_data    *errd;
    struct RastPort 	    	    *rp;
    
    errd = (struct eraserect_render_data *)err_data;
    
    rp = errd->origrp;
     
    msg.Layer	= rp->Layer;
    msg.MinX	= x1;
    msg.MinY	= y1;
    msg.MaxX	= x2;
    msg.MaxY	= y2;
     
    #warning What should these be set to ?
    msg.OffsetX = 0;
    msg.OffsetY = 0;
     
    if (NULL != msg.Layer)
    {
    	struct RastPort *rp = NULL;
	
        if (!errd->rsi.onscreen)
	{
	    if (NULL == errd->fakerp)
		errd->fakerp = CreateRastPort();
	    if (NULL == errd->fakerp)
		return 0;
		
	    rp = errd->fakerp;
	    rp->BitMap = errd->rsi.curbm;
	    
	}
	else
	{
	    rp = errd->origrp;
	}
        
     	calllayerhook(msg.Layer->BackFill, rp, &msg, GfxBase);
    }
    
    return 0;
}

/****************************************************************************************/

