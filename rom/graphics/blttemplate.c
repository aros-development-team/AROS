/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
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

static ULONG blttemplate_render(APTR btr_data, WORD srcx, WORD srcy,
    	    	    	    	OOP_Object *dstbm_obj, OOP_Object *dst_gc,
    	    	    	    	struct Rectangle *rect, struct GfxBase *GfxBase)
{
    struct bt_render_data *btrd = btr_data;
    WORD   	    	   width  = rect->MaxX - rect->MinX + 1;
    WORD		   height = rect->MaxY - rect->MinY + 1;
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
	AROS_LHA(WORD              	, xSrc		, D0),
	AROS_LHA(WORD              	, srcMod	, D1),
	AROS_LHA(struct RastPort * 	, destRP	, A1),
	AROS_LHA(WORD              	, xDest		, D2),
	AROS_LHA(WORD              	, yDest		, D3),
	AROS_LHA(WORD              	, xSize		, D4),
	AROS_LHA(WORD              	, ySize		, D5),

/*  LOCATION */
	struct GfxBase *, GfxBase, 6, Graphics)
	
/*  FUNCTION
	Draws part of a single-bitplane image into the RastPort in the current
	colors (foreground and background) and drawing mode.

    INPUTS
	source - pointer to the aligned UWORD in which the top-lefthand corner
	    of the template is located.
	xSrc - bit offset of top-lefthand corner of template from start of
	    UWORD pointed to by 'source' input (0 to 15).
	srcMod - number of bytes per row in template's bitplane.
	destRP - destination RastPort.
	xDest,yDest - upper left corner of destination.
	xSize,ySize - size of destination.

    RESULT
	None.

    NOTES
	The size and destination coordinates may be outside the RastPort
	boundaries, in which case the affected area is safely truncated.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct bt_render_data btrd;
    struct Rectangle 	  rr;

    EnterFunc(bug("driver_BltTemplate(%d, %d, %d, %d, %d, %d)\n"
    	, xSrc, srcMod, xDest, yDest, xSize, ySize));
	
    FIX_GFXCOORD(xDest);
    FIX_GFXCOORD(yDest);

    btrd.template  	 = (UBYTE *)source;
    btrd.srcx	    	 = xSrc;
    btrd.modulo    	 = srcMod;
    btrd.inverttemplate = (destRP->DrawMode & INVERSVID) ? TRUE : FALSE;
    
    rr.MinX = xDest;
    rr.MinY = yDest;
    rr.MaxX = xDest + xSize  - 1;
    rr.MaxY = yDest + ySize - 1;

    do_render_func(destRP, NULL, &rr, blttemplate_render, &btrd, TRUE, FALSE, GfxBase);    
    ReturnVoid("driver_BltTemplate");
    
    AROS_LIBFUNC_EXIT
    
} /* BltTemplate */
