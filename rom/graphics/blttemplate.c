/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/****************************************************************************************/

struct bt_render_data
{
    UBYTE *template;
    ULONG  modulo;
    WORD   srcx;
    UBYTE  inverttemplate;
};

static ULONG blttemplate_render(APTR btr_data, LONG srcx, LONG srcy,
    	    	    	    	OOP_Object *dstbm_obj, OOP_Object *dst_gc,
    	    	    	    	struct Rectangle *rect, struct GfxBase *GfxBase)
{
    struct bt_render_data *btrd = btr_data;
    ULONG   	    	   width  = rect->MaxX - rect->MinX + 1;
    ULONG		   height = rect->MaxY - rect->MinY + 1;
    UBYTE		   x = srcx + btrd->srcx;
    UBYTE   	    	  *template = btrd->template + btrd->modulo * srcy;
    
    HIDD_BM_PutTemplate(dstbm_obj, dst_gc, template, btrd->modulo,
    	    	    	x, rect->MinX, rect->MinY, width, height, btrd->inverttemplate);

    return width * height;
}
			 
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
	Draws the image in the template into the
	RastPort in the current color and drawing mode.

    INPUTS
	source - template bitplane. Should be Word aligned.
	xSrc - x offset in source plane (0...15).
	srcMod - BytesPerRow in template mask.
	destRP - destination RastPort.
	xDest,yDest - upper left corner of destination.
	xSize,ySize - size of destination.

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
    
    struct bt_render_data btrd;
    struct Rectangle 	  rr;

    EnterFunc(bug("driver_BltTemplate(%d, %d, %d, %d, %d, %d)\n"
    	, xSrc, srcMod, xDest, yDest, xSize, ySize));
	
    FIX_GFXCOORD(xDest);
    FIX_GFXCOORD(yDest);
	
    if (!OBTAIN_DRIVERDATA(destRP, GfxBase))
    	ReturnVoid("driver_BltTemplate");
	
    btrd.template  	 = (UBYTE *)source;
    btrd.srcx	    	 = xSrc;
    btrd.modulo    	 = srcMod;
    btrd.inverttemplate = (destRP->DrawMode & INVERSVID) ? TRUE : FALSE;
    
    rr.MinX = xDest;
    rr.MinY = yDest;
    rr.MaxX = xDest + xSize  - 1;
    rr.MaxY = yDest + ySize - 1;
    
    do_render_func(destRP, NULL, &rr, blttemplate_render, &btrd, TRUE, FALSE, GfxBase);
	
    RELEASE_DRIVERDATA(destRP, GfxBase);
    
    ReturnVoid("driver_BltTemplate");
    
    AROS_LIBFUNC_EXIT
    
} /* BltTemplate */
