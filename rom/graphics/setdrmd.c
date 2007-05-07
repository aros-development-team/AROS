/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$    $Log

    Desc: Graphics function SetDrMd()
    Lang: english
*/
#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include <proto/oop.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH2(void, SetDrMd,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(ULONG            , drawMode, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 59, Graphics)

/*  FUNCTION
	Set the drawing mode for lines, fills and text.

    INPUTS
	rp       - RastPort
	drawMode - see graphics/rastport.h for possible flags.

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

    struct TagItem drmd_tags[] =
    {
	{ aHidd_GC_ColorExpansionMode	, 0UL 	    	    	 },
	{ aHidd_GC_DrawMode 	    	, vHidd_GC_DrawMode_Copy },
	{ TAG_DONE  	    	    	    	    	    	 }
    };
    struct gfx_driverdata *dd;
    
    if (OBTAIN_DRIVERDATA(rp, GfxBase))
    {	
	if (drawMode & JAM2)
	{
    	    drmd_tags[0].ti_Data = vHidd_GC_ColExp_Opaque;
	}	
	else if (drawMode & COMPLEMENT)
	{
	    drmd_tags[1].ti_Data = vHidd_GC_DrawMode_Invert;
	}
	else if ((drawMode & (~INVERSVID)) == JAM1)
	{
    	    drmd_tags[0].ti_Data = vHidd_GC_ColExp_Transparent;
	}

    	#warning Handle INVERSVID by swapping apen and bpen ?

	dd = GetDriverData(rp);
	if (dd)
	{
    	    OOP_SetAttrs(dd->dd_GC, drmd_tags);
	}
	RELEASE_DRIVERDATA(rp, GfxBase);
    }

    rp->DrawMode = drawMode;
    rp->linpatcnt = 15;

    AROS_LIBFUNC_EXIT
} /* SetDrMd */
